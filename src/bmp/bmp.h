#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stdio.h>
#include "../image_common.h"

struct __attribute__((packed)) bmp_fileHeader {
    char signature[2];
    uint32_t size;
    uint16_t reserve1;
    uint16_t reserve2;
    uint32_t offset;
};

struct __attribute__((packed)) bmp_bitmapInfoHeader {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
    int32_t horizontalRes;
    int32_t verticalRes;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};

void bmp_printFileHeader(struct bmp_fileHeader *header);
void bmp_printBitmapInfoHeader(struct bmp_bitmapInfoHeader *header);
void bmp_printPixels(void *pixels, struct bmp_bitmapInfoHeader *header);
void* bmp_mallocPixels(int width, int height, size_t size);
int bmp_readFileHeader(FILE *fptr, struct bmp_fileHeader *header);
int bmp_readBitmapInfoHeader(FILE *fptr, struct bmp_bitmapInfoHeader *header);
void* bmp_readPixels(FILE *fptr, struct bmp_bitmapInfoHeader *header);
void bmp_open(char filename[]);

#endif  // BMP_H
