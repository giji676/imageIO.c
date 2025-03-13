#ifndef PNG_H
#define PNG_H

#include <stdint.h>
#include <stdio.h>
#include "../image_common.h"

struct __attribute__((packed)) png_fileSignature {
    char signature[8];
};

struct __attribute__((packed)) png_chunkLayout {
    uint32_t length;
    char chunkType[4];
    void *chunkData;
    uint32_t crc;
};

struct __attribute__((packed)) png_IHDR {
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colorType;
    uint8_t compressionMethod;
    uint8_t filterMethod;
    uint8_t interlaceMethod;
};

void png_printIHDR(struct png_IHDR *ihdr);
void png_printFileSignature(struct png_fileSignature *fileSignature);
void png_printChunkLayout(struct png_chunkLayout *chunkLayout);
int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature);
int png_readChunkLayout(FILE *fptr, struct png_chunkLayout *chunkLayout);
void png_open(char filename[]);

#endif  // PNG_H
