#include "./png_write.h"
#include "./png.h"
#include "../crc/crc.h"
#include "../display/display.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct huffmanCode fixed_huffman_code(uint16_t symbol) {
    struct huffmanCode hc;
    if (symbol <= 143) {
        hc.code = 0b00110000 + (symbol - 0);
        hc.length = 8;
    } else if (symbol <= 255) {
        hc.code = 0b110010000 + (symbol - 144);
        hc.length = 9;
    } else if (symbol <= 279) {
        hc.code = 0b0000000 + (symbol - 256);
        hc.length = 7;
    } else if (symbol <= 287) {
        hc.code = 0b11000000 + (symbol - 280);
        hc.length = 8;
    } else {
        hc.code = 0;
        hc.length = 0; // invalid
    }
    return hc;
}

void fixed_huffman(struct bitStream *bs, uint8_t *data, int size) {
    for (int i = 0; i < size; i++) {
        struct huffmanCode hc = fixed_huffman_code(data[i]);
        hc.code = reverse_bits(hc.code, hc.length);
        bitstream_write(bs, hc.length, hc.code);
    }

    struct huffmanCode eob = fixed_huffman_code(256);
    eob.code = reverse_bits(eob.code, eob.length);
    bitstream_write(bs, eob.length, eob.code);

    // Flush to next byte
    bitstream_flush(bs);
}

void png_deflate(struct png_image *image, struct png_IDAT *idat, FILE *fptr) {
    // 00 0000 16  49 4441 54   08  99 
    // |  size  |  |I  D A T| |CMF|FLG|
    // 63 f8cf c0c0 f09f 8191 81e1 ffff ff0c 001e f604 fd 
    // |                        DATA                    |
    // 6f e848 5e
    // |   CRC  |

    int chunkDataSize = 2 + 22 + 4; // cmf,flg,data,adler
    uint8_t *chunkData = malloc(chunkDataSize);
    struct bitStream bs;
    bitstream_init(&bs, chunkData, chunkDataSize);

    idat->cm = 8;
    idat->cinfo = 0;
    idat->cmf = (idat->cinfo << 4) | idat->cm;
    idat->flevel = 2;
    idat->fdict = 0;
    idat->fcheck = 31 - ((idat->cmf << 8 | (idat->flevel << 1) | idat->fdict)) % 31;
    idat->flg = (idat->flevel << 1 | idat->fdict) << 5 | idat->fcheck;
    bitstream_write(&bs, 4, idat->cm);
    bitstream_write(&bs, 4, idat->cinfo);
    bitstream_write(&bs, 5, idat->fcheck);
    bitstream_write(&bs, 1, idat->fdict);
    bitstream_write(&bs, 2, idat->flevel);
    bitstream_write(&bs, 1, 1); // BFINAL 0b1
    bitstream_write(&bs, 2, 1); // BTYPE  0b01

    int bpp = 3; // assuming RGB 8-bit
    int row_bytes = image->ihdr.width * bpp;
    idat->data_length = (row_bytes + 1) * image->ihdr.height;
    idat->data = malloc(idat->data_length);

    for (uint32_t i = 0; i < image->ihdr.height; i++) {
        int row_start = i * (row_bytes + 1); // +1 for filter byte

        if (i == 0) {
            // Row 0: None
            idat->data[row_start] = 0;
            for (int x = 0; x < row_bytes; x++) {
                idat->data[row_start + 1 + x] = image->pixels[x];
            }
        } else if (i >= 1) {
            // Row 1: Sub
            idat->data[row_start] = 1; // Sub filter
            for (int x = 0; x < row_bytes; x++) {
                uint8_t recon;
                if (x >= bpp) {
                    uint8_t raw = image->pixels[i * row_bytes + x];
                    uint8_t left = image->pixels[i * row_bytes + x - bpp];
                    recon = raw - left;
                } else {
                    recon = image->pixels[i * row_bytes + x];
                }
                idat->data[row_start + 1 + x] = recon;
            }
        }
    }
    int outSize = (image->ihdr.width * bpp + 1) * image->ihdr.height;
    printf("Expected:\n");
    printf("00 FF 00 00 00 FF 00 01 00 00 FF FF FF 00\n");
    printf("Result:\n");
    for (int i = 0; i < outSize; i++) {
        printf("%02X ", idat->data[i]);
        if (i > 0 && i % 16 == 15) {
            printf("\n");
        }
    }
    printf("\n");

    uint32_t a = 1;
    uint32_t b = 0;

    for (size_t i = 0; i < outSize; i++) {
        a = (a + idat->data[i]) % 65521;
        b = (b + a) % 65521;
    }

    uint32_t calculated_adler32 = (b << 16) | a;
    uint32_t swapped_adler32 = __builtin_bswap32(calculated_adler32);

    fixed_huffman(&bs, idat->data, outSize);
    fwrite(bs.data, bitstream_get_size(&bs), 1, fptr);
    fwrite(&swapped_adler32, 4, 1, fptr);
}

int png_save(char filename[]) {
    FILE *fptr;
    if ((fptr = fopen(filename, "wb")) == NULL) {
        printf("Failed to open file %s for writing\n", filename);
        return -1;
    }

    struct png_fileSignature png_fileSignature = {
        .signature = {'\211','P','N','G','\r','\n','\032','\n'}
    };
    // fwrite(&png_fileSignature, sizeof(png_fileSignature), 1, fptr);

    struct png_IHDR ihdr = {
        .width = 2,
        .height = 2,
        .bitDepth = 8,
        .colorType = 2,
        .compressionMethod = 0,
        .filterMethod = 0,
        .interlaceMethod = 0,
    };

    struct png_chunk ihdr_chunk = {
        .length = sizeof(struct png_IHDR),
        .chunkType = {'I','H','D','R'},
        .chunkData = &ihdr,
    };
    ihdr.width = __builtin_bswap32(ihdr.width);
    ihdr.height = __builtin_bswap32(ihdr.height);
    png_calculateCRC(&ihdr_chunk);
    ihdr.width = __builtin_bswap32(ihdr.width);
    ihdr.height = __builtin_bswap32(ihdr.height);

    struct png_image image;
    image.ihdr = ihdr;

    uint8_t *pixelData = malloc(2*2*3);
    if (pixelData == NULL) {
        printf("Failed to allocate memory for raw image pixel data\n");
        return -1;
    }

    memcpy(pixelData, (uint8_t[]){
        0xFF, 0x00, 0x00,
        0x00, 0xFF, 0x00,
        0x00, 0x00, 0xFF,
        0xFF, 0xFF, 0xFF
    }, 2 * 2 * 3);
    image.pixels = pixelData;
    image.pixel_size = ihdr.width * ihdr.height * 3;

    struct png_IDAT idat;
    struct png_chunk idat_chunk = {
        .length = 22, // TEMP
        .chunkType = {'I','D','A','T'},
        .chunkData = &idat,
    };

    png_deflate(&image, &idat, fptr);
    // write_chunk(fptr, &idat_chunk);
    free(idat.data);

    fclose(fptr);
    return 1;
}

