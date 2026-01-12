#include "./png.h"
#include "../crc/crc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void png_printPixels(void *pixels, struct png_IHDR *ihdr) {
    for (uint32_t i = 0; i < ihdr->height; i++) {
        for (uint32_t j = 0; j < ihdr->width; j++) {
            if (ihdr->bitDepth == 8 && ihdr->colorType == 2) {
                struct rgbPixel *pixel = &((struct rgbPixel *)pixels)[i * ihdr->width + j];
                printf("(%d,%d,%d)", pixel->r, pixel->g, pixel->b);
            }
            printf(" ");
        }
        printf("\n");
    }
}

int png_compareAdler32(struct png_IDAT *idat, uint8_t *output, size_t output_pos) {
    uint32_t a = 1;
    uint32_t b = 0;

    for (size_t i = 0; i < output_pos; i++) {
        a = (a + output[i]) % 65521;
        b = (b + a) % 65521;
    }

    uint32_t calculated_adler32 = (b << 16) | a;
    if (calculated_adler32 != idat->adler32) {
        return -1;
    }
    return 1;
}

void png_printIDAT(struct png_IDAT *idat) {
    printf("CMF: 0x%02X\n", idat->cmf);
    printf("CM: %u\n", idat->cm);
    printf("CINFO: %u\n", idat->cinfo);
    printf("FLG: 0x%02X\n", idat->flg);
    printf("FCHECK: %u\n", idat->fcheck);
    printf("FDICT: %u\n", idat->fdict);
    printf("FLEVEL: %u\n", idat->flevel);
    printf("DATA_LENGTH: %u\n", idat->data_length);
    printf("DATA: ...\n");
    // for (uint32_t i = 0; i < idat->data_length; ++i) {
    //     printf("%02x ", idat->data[i]);
    //     if (i > 0 && i % 16 == 15) {
    //         printf("\n");
    //     }
    // }
    // printf("\n");
    printf("ADLER32: 0x%08X\n", idat->adler32);
}

int png_readIDAT(void *data, uint32_t length, struct png_IDAT *idat) {
    struct bitStream bs;
    bitstream_init(&bs, (uint8_t *)data, length);

    uint8_t cmf = ((uint8_t *)data)[0];
    uint8_t flg = ((uint8_t *)data)[1];

    if (((cmf << 8) | flg) % 31 != 0) {
        printf("Invalid zlib header\n");
        return -1;
    }

    uint32_t cm, cinfo;
    uint32_t fcheck, fdict, flevel;
    bitstream_read(&bs, 4, &cm);
    bitstream_read(&bs, 4, &cinfo);
    bitstream_read(&bs, 5, &fcheck);
    bitstream_read(&bs, 1, &fdict);
    bitstream_read(&bs, 2, &flevel);

    uint32_t adler32 =
        ((uint32_t)((uint8_t *)data)[length - 4] << 24) |
        ((uint32_t)((uint8_t *)data)[length - 3] << 16) |
        ((uint32_t)((uint8_t *)data)[length - 2] << 8)  |
        ((uint32_t)((uint8_t *)data)[length - 1]);

    // No need to do -2 (for the CM and CINFO bytes)
    // as we are already skipping them by starting data+2
    int data_length = length - 4; // ADLER32: 4 bytes
    if (data_length < 0) {
        printf("Invalid zlib data length\n");
        return -1;
    }
    idat->cmf = cmf;
    idat->flg = flg;
    idat->cm = cm;
    idat->cinfo = cinfo;
    idat->fcheck = fcheck;
    idat->fdict = fdict;
    idat->flevel = flevel;
    idat->data_length = data_length;
    idat->data = (uint8_t *)data+2;
    idat->adler32 = adler32;
    return 1;
}

uint8_t *png_processIDAT(void *data, uint32_t length,
                         struct png_IHDR *ihdr,
                         size_t *out_size) {
    int width  = ihdr->width;
    int height = ihdr->height;
    int bpp;
    switch (ihdr->colorType) {
        case 2:
            if (ihdr->bitDepth != 8) {
                printf("Unsupported bit depth %u for color type 2\n", ihdr->bitDepth);
                return NULL;
            }
            bpp = 3;
            break;
        default:
            printf("Unsupported color type %u\n", ihdr->colorType);
            return NULL;
    }

    struct png_IDAT idat;
    png_readIDAT(data, length, &idat);
    // png_printIDAT(&idat);

    struct bitStream ds;
    bitstream_init(&ds, idat.data, idat.data_length);
    uint32_t bfinal, btype;
    bitstream_read(&ds, 1, &bfinal);
    bitstream_read(&ds, 2, &btype);

    // printf("BFINAL: %u ", bfinal);
    // printf("BTYPE: %u\n", btype);

    // Lit Value    Bits        Codes
    // ---------    ----        -----
    //   0 - 143     8          00110000 through
    //                          10111111
    // 144 - 255     9          110010000 through
    //                          111111111
    // 256 - 279     7          0000000 through
    //                          0010111
    // 280 - 287     8          11000000 through
    //                          11000111

    size_t expected = height * (width * bpp + 1);
    uint8_t *output = malloc(expected);
    if (!output) {
        printf("Failed to allocate output buffer\n");
        return NULL;
    }
    size_t output_pos = 0;

    while (1) {
        uint32_t symbol = 0;
        uint32_t peeked;

        bitstream_peek(&ds, 9, &peeked);

        uint32_t rev7 = reverse_bits(peeked, 7);
        if (rev7 <= 23) {                 // 256–279
            symbol = 256 + rev7;
            bitstream_read(&ds, 7, &peeked);
        } else {
            uint32_t rev8 = reverse_bits(peeked, 8);
            if (rev8 >= 0x30 && rev8 <= 0xBF) {          // 0–143
                symbol = rev8 - 0x30;
                bitstream_read(&ds, 8, &peeked);
            } else if (rev8 >= 0xC0 && rev8 <= 0xC7) {   // 280–287
                symbol = 280 + (rev8 - 0xC0);
                bitstream_read(&ds, 8, &peeked);
            } else {
                uint32_t rev9 = reverse_bits(peeked, 9); // 144–255
                if (rev9 >= 0x190 && rev9 <= 0x1FF) {
                    symbol = 144 + (rev9 - 0x190);
                    bitstream_read(&ds, 9, &peeked);
                } else {
                    printf("Invalid Huffman symbol\n");
                    free(output);
                    return NULL;
                }
            }
        }

        /* ---- termination ---- */
        if (symbol == 256) {
            // printf("End of block symbol encountered\n");
            break;
        }

        /* ---- literal ---- */
        if (symbol < 256) {
            if (output_pos >= expected) {
                printf("Output buffer overflow\n");
                free(output);
                return NULL;
            }
            output[output_pos++] = (uint8_t)symbol;
            continue;
        }

        /* ---- length/distance (not implemented yet) ---- */
        printf("Length/distance symbol %u encountered (not handled yet)\n", symbol);
        free(output);
        return NULL;
    }

    if (!png_compareAdler32(&idat, output, output_pos)) {
        printf("Adler32 NOT MATCHING\n");
    }
    int row_bytes = bpp * width + 1;

    uint8_t *final_output = malloc(width * height * bpp);
    int idx = 0;

    for (int row = 0; row < height; row++) {
        int row_start = row * row_bytes;
        uint8_t filter = output[row_start];

        for (int i = 0; i < bpp * width; i++) {
            uint8_t raw = output[row_start + 1 + i];
            uint8_t recon;

            if (filter == 0) { // None
                recon = raw;
            }
            else if (filter == 1) { // Sub
                uint8_t left = 0;
                if (i >= bpp) {
                    left = final_output[idx - bpp];
                }
                recon = raw + left;
            }

            final_output[idx++] = recon;
        }
    }

    free(output);

    *out_size = width * height * bpp;
    return final_output;
}

void png_printIHDR(struct png_IHDR *ihdr) {
    uint32_t width = __builtin_bswap32(ihdr->width);
    uint32_t height = __builtin_bswap32(ihdr->height);
    printf("width: %u\n", width);
    printf("height: %u\n", height);
    printf("bitDepth: %u\n", ihdr->bitDepth);
    printf("colorType: %u\n", ihdr->colorType);
    printf("compressionMethod: %u\n", ihdr->compressionMethod);
    printf("filterMethod: %u\n", ihdr->filterMethod);
    printf("interlaceMethod: %u", ihdr->interlaceMethod);
}

void png_printChunk(struct png_chunk *chunk, struct png_image *image) {
    printf("\n");
    printf("Chunk\n");
    printf("length: %u\n", chunk->length);
    printf("chunkType: %.4s\n", chunk->chunkType);
    if (strncmp(chunk->chunkType, "IHDR", 4) == 0) {
        memcpy(&image->ihdr, chunk->chunkData, sizeof(struct png_IHDR));

        image->ihdr.width  = __builtin_bswap32(image->ihdr.width);
        image->ihdr.height = __builtin_bswap32(image->ihdr.height);

        png_printIHDR((struct png_IHDR *)chunk->chunkData);
    } else if (strncmp(chunk->chunkType, "IDAT", 4) == 0) {
            image->pixels = png_processIDAT(
                chunk->chunkData,
                chunk->length,
                &image->ihdr,
                &image->pixel_size
            );
            if (image->pixels == NULL) {
                printf("Failed to process IDAT chunk\n");
            }
        } else if (strncmp(chunk->chunkType, "zTXt", 4) == 0) {
        printf("zTXt chunk data: ...");
        // png_interpretzTXt(chunk->chunkData, chunk->length);
    } else {
        printf("chunkData:");
        for (uint32_t i = 0; i < chunk->length; ++i) {
            if (i % 16 == 0) {
                printf("\n");
            }
            printf("%02x ", ((unsigned char *)chunk->chunkData)[i]);
        }
    }
    printf("\n");
    // printf("expected crc: 0x%08X\n", chunk->crc);
    if (!png_compareCRC(chunk)) {
        printf("CRC NOT MATCHING\n");
    }
}

int png_compareCRC(struct png_chunk *chunk) {
    int crc_inp_len = sizeof(chunk->chunkType) + (int)chunk->length;
    unsigned char *buff = malloc(crc_inp_len);
    if (buff == NULL) {
        printf("Failed to allocate memory for chunk crc buffer\n");
        return -1;
    }
    memcpy(buff, chunk->chunkType, sizeof(chunk->chunkType));
    memcpy(buff + sizeof(chunk->chunkType), chunk->chunkData, chunk->length);
    // printf("raw buff:\n");
    // for (int i = 0; i < crc_inp_len; i++) {
    //     printf("0x%02X ", buff[i]);
    // }
    // printf("\n");

    unsigned long res = crc(buff, crc_inp_len);
    // printf("calculated crc: 0x%lX\n", res);
    free(buff);
    if (res == chunk->crc) {
        return 1;
    } else {
        return 0;
    }
}

void png_interpretzTXt(void *data, uint32_t length) {
    if (data == NULL || length == 0) {
        printf("Invalid zTXt data\n");
        return;
    }

    unsigned char *bytes = (unsigned char *)data;
    struct png_zTXt ztxt;
    ztxt.keyword = (char *)bytes;

    uint32_t keyword_end = 0;

    while (keyword_end < length && bytes[keyword_end] != 0) {
        keyword_end++;
    }

    if (keyword_end >= length || keyword_end + 1 >= length) {
        printf("Invalid zTXt chunk format\n");
        return;
    }
    ztxt.compMethod = &bytes[keyword_end + 1];
    ztxt.compText = (char *)&bytes[keyword_end + 2];

    printf("Keyword: %s\n", ztxt.keyword);
    printf("Compression Method: %u\n", *ztxt.compMethod);

    uint32_t compText_length = length - keyword_end - 2;

    printf("Compressed Text:\n");

    for (uint32_t i = 0; i < compText_length; i++) {
        if (i > 0 && i % 16 == 0) {
            printf("\n");
        }
        printf("%02x ", (unsigned char)ztxt.compText[i]);
    }
}

void png_printFileSignature(struct png_fileSignature *fileSignature) {
    printf("\n");
    printf("File Signature\n");
    printf("signature: ");
    for (int i = 0; i < (int)sizeof(fileSignature->signature); ++i) {
        printf("%02x ", (unsigned char)fileSignature->signature[i]);
    }
    printf("\n");
}

int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature) {
    if (fread(fileSignature, sizeof(struct png_fileSignature), 1, fptr) != 1) {
        printf("Failed to read file signature\n");
        return -1;
    }
    return 1;
}

int png_readChunk(FILE *fptr, struct png_chunk *chunk) {
    if (fread(chunk, (sizeof(chunk->length) + sizeof(chunk->chunkType)), 1,
              fptr) != 1) {
        printf("Failed to read chunk layout\n");
        return -1;
    }

    chunk->length = __builtin_bswap32(chunk->length);
    chunk->chunkData = (void *)malloc(chunk->length);
    if (chunk->chunkData == NULL) {
        printf("Failed to allocate memory for chunk data\n");
        return -1;
    }
    if (chunk->length == 0) {
        chunk->chunkData = NULL;
        // MEM LEAK HERE?
    } else if (fread(chunk->chunkData, chunk->length, 1, fptr) != 1) {
        printf("Failed to read chunk data\n");
        free(chunk->chunkData);
        return -1;
    }

    if (strncmp(chunk->chunkType, "IHDR", 4) == 0 && chunk->length == 13) {
        /*
    ((struct png_IHDR *)chunk->chunkData)->width =
        __builtin_bswap32(((struct png_IHDR *)chunk->chunkData)->width);
    ((struct png_IHDR *)chunk->chunkData)->height =
        __builtin_bswap32(((struct png_IHDR *)chunk->chunkData)->height);
    */
    } else if (strncmp(chunk->chunkType, "zTXt", 4) == 0) {
        // png_interpretzTXt(chunk->chunkData, chunk->length);
    }

    if (fread(&chunk->crc, sizeof(chunk->crc), 1, fptr) != 1) {
        printf("Failed to read chunk crc\n");
        free(chunk->chunkData);
        return -1;
    }
    chunk->crc = __builtin_bswap32(chunk->crc);

    return 1;
}

int png_readChunks(FILE *fptr, struct png_chunk **chunks, struct png_image *image) {
    int chunkCount = 0;

    while (1) {
        if (chunkCount > 0) {
            size_t cSize = (chunkCount + 1) * sizeof(struct png_chunk);
            struct png_chunk *temp = realloc(*chunks, cSize);
            if (temp == NULL) {
                printf("Failed to allocte memory for chunks\n");
                break;
            }
            *chunks = temp;
        }
        if (png_readChunk(fptr, &(*chunks)[chunkCount]) != 1) {
            printf("Error reading chunk or end of file\n");
            break;
        }
        png_printChunk(&(*chunks)[chunkCount], image);

        if (strncmp((*chunks)[chunkCount].chunkType, "IEND", 4) == 0) {
            printf("\nEnd of file reached\n");
            chunkCount++;
            break;
        }
        chunkCount++;
    }

    return chunkCount;
}

void png_open(char filename[]) {
    FILE *fptr;

    make_crc_table();

    if ((fptr = fopen(filename, "rb")) == NULL) {
        printf("Failed to open file %s\n", filename);
        return;
    }

    struct png_fileSignature png_fileSignature;
    if (png_readFileSignature(fptr, &png_fileSignature) != 1) {
        return;
    }
    png_printFileSignature(&png_fileSignature);

    struct png_chunk *chunks = malloc(sizeof(struct png_chunk));
    if (chunks == NULL) {
        printf("Failed to allocte memory for chunks\n");
        return;
    }

    struct png_image image = {0};
    int chunkCount = png_readChunks(fptr, &chunks, &image);
    if (chunkCount < 0) {
        printf("Error reading chunks\n");
        fclose(fptr);
        return;
    }
    fclose(fptr);

    png_printPixels(image.pixels, &image.ihdr);
    free(image.pixels);

    for (int i = 0; i < chunkCount; ++i) {
        free(chunks[i].chunkData);
    }
    free(chunks);
}
