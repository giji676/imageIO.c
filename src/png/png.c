#include "./png.h"
#include "../crc/crc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_OUTPUT 1024

// Reverse the lowest n bits of x
uint32_t reverse_bits(uint32_t x, int n) {
    uint32_t r = 0;
    for (int i = 0; i < n; i++) {
        r <<= 1;
        r |= (x >> i) & 1;
    }
    return r;
}

void png_printIDAT(void *data, uint32_t length) {
    FILE *f = fopen("idat.zlib", "wb");
    fwrite(data, 1, length, f);
    fclose(f);
    printf("IDAT chunk data (%u bytes):\n", length);
    for (uint32_t i = 0; i < length; ++i) {
        if (i % 16 == 0 && i != 0) {
            printf("\n");
        }
        printf("%02x ", ((unsigned char *)data)[i]);
    }
    printf("\n");

    uint8_t cmf = ((uint8_t *)data)[0];
    uint8_t flg = ((uint8_t *)data)[1];
    if ((cmf * 256 + flg) % 31 != 0) {
        printf("Invalid zlib header\n");
        return;
    }
    printf("CMF: 0x%02X\n", cmf);
    printf("FLG: 0x%02X\n", flg);
    if ((cmf * 256 + (uint16_t)flg) % 31 != 0) {
        printf("Invalid zlib header: CMF and FLG are not consistent\n");
        return;
    }

    struct bitStream cmf_bs;
    bitstream_init(&cmf_bs, &cmf, sizeof(cmf));
    uint32_t cm, cinfo;
    bitstream_read(&cmf_bs, 4, &cm);
    bitstream_read(&cmf_bs, 4, &cinfo);
    printf("CM: %u ", cm);
    printf("CINFO: %u\n", cinfo);

    struct bitStream flg_bs;
    bitstream_init(&flg_bs, &flg, sizeof(cmf));
    uint32_t fcheck, fdict, flevel;
    bitstream_read(&flg_bs, 5, &fcheck);
    bitstream_read(&flg_bs, 1, &fdict);
    bitstream_read(&flg_bs, 2, &flevel);
    printf("FCHECK: %u ", fcheck);
    printf("FDICT: %u ", fdict);
    printf("FLEVEL: %u\n", flevel);

    uint8_t *deflate_data = (uint8_t *)data + 2;
    size_t deflate_length = length - 4; // minus 4 bytes Adler32

    struct bitStream ds;
    bitstream_init(&ds, deflate_data, deflate_length);
    uint32_t bfinal, btype;
    bitstream_read(&ds, 1, &bfinal);
    bitstream_read(&ds, 2, &btype);

    printf("BFINAL: %u ", bfinal);
    printf("BTYPE: %u\n", btype);

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

    uint8_t output[MAX_OUTPUT];
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
                    return;
                }
            }
        }

        /* ---- termination ---- */
        if (symbol == 256) {
            printf("End of block symbol encountered\n");
            break;
        }

        /* ---- literal ---- */
        if (symbol < 256) {
            if (output_pos >= MAX_OUTPUT) {
                printf("Output buffer overflow\n");
                return;
            }
            output[output_pos++] = (uint8_t)symbol;
            continue;
        }

        /* ---- length/distance (not implemented yet) ---- */
        printf("Length/distance symbol %u encountered (not handled yet)\n", symbol);
        return;
    }

    uint32_t a = 1;
    uint32_t b = 0;

    for (size_t i = 0; i < output_pos; i++) {
        a = (a + output[i]) % 65521;
        b = (b + a) % 65521;
    }

    uint32_t calculated = (b << 16) | a;
    printf("Calculated Adler32: 0x%08X\n", calculated);
    uint32_t expected =
        ((uint32_t)((uint8_t *)data)[length - 4] << 24) |
        ((uint32_t)((uint8_t *)data)[length - 3] << 16) |
        ((uint32_t)((uint8_t *)data)[length - 2] << 8)  |
        ((uint32_t)((uint8_t *)data)[length - 1]);

    printf("Expected Adler32:   0x%08X\n", expected);

    int bpp = 3;
    int width = 2;
    int height = 2;
    int row_bytes = bpp * width + 1;

    uint8_t *final_output = malloc(width * height * bpp);
    int idx = 0;

    for (int row = 0; row < height; row++) {
        int row_start = row * row_bytes;
        uint8_t filter = output[row_start];

        for (int i = 0; i < bpp * width; i++) {
            uint8_t raw = output[row_start + 1 + i];
            uint8_t recon;

            if (filter == 0) {
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

    for (int i = 0; i < width * height * bpp; i++) {
        printf("%02X ", final_output[i]);
    }
    printf("\n");

    free(final_output);
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

void png_printChunk(struct png_chunk *chunk) {
    printf("\n");
    printf("Chunk\n");
    printf("length: %u\n", chunk->length);
    printf("chunkType: %.4s\n", chunk->chunkType);
    if (strncmp(chunk->chunkType, "IHDR", 4) == 0 && chunk->length == 13) {
        png_printIHDR((struct png_IHDR *)chunk->chunkData);
    } else if (strncmp(chunk->chunkType, "IDAT", 4) == 0) {
        png_printIDAT((struct png_IDAT *)chunk->chunkData, chunk->length);
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
    printf("expected crc: 0x%08X\n", chunk->crc);
    int crc_compare_res = png_compareCRC(chunk);
    if (crc_compare_res == 1) {
        // printf("CRC MATCHING\n");
    } else {
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
    /*
  printf("raw buff:\n");
  for (int i = 0; i < crc_inp_len; i++) {
      printf("0x%02X ", buff[i]);
  }
  printf("\n");
  */

    unsigned long res = crc(buff, crc_inp_len);
    printf("calculated crc: 0x%lX\n", res);
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

int png_readChunks(FILE *fptr, struct png_chunk **chunks) {
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
        png_printChunk(&(*chunks)[chunkCount]);

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

    int chunkCount = png_readChunks(fptr, &chunks);
    if (chunkCount < 0) {
        printf("Error reading chunks\n");
        fclose(fptr);
        return;
    }
    fclose(fptr);

    for (int i = 0; i < chunkCount; ++i) {
        free(chunks[i].chunkData);
    }
    free(chunks);
}
