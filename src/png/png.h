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
    uint8_t *chunkData;
    uint32_t crc;
};

void png_printFileSignature(struct png_fileSignature *fileSignature);
void png_printChunkLayout(struct png_chunkLayout *chunkLayout);
int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature);
int png_readChunkLayout(FILE *fptr, struct png_chunkLayout *chunkLayout);
void png_open(char filename[]);

#endif  // PNG_H
