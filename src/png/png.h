#ifndef PNG_H
#define PNG_H

#include <stdint.h>
#include <stdio.h>
#include "../image_common.h"

struct __attribute__((packed)) png_fileSignature {
    char signature[8];
};

struct __attribute__((packed)) png_chunk {
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

struct png_zTXt {
    char *keyword;
    uint8_t *compMethod;
    char *compText;
};

void png_printIHDR(struct png_IHDR *ihdr);
void png_printIDAT(void *data, uint32_t length);
void png_printzTXt(void *data);
void png_printFileSignature(struct png_fileSignature *fileSignature);
void png_printChunk(struct png_chunk *chunk);
int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature);
int png_readChunks(FILE *fptr, struct png_chunk **chunk);
int png_readChunk(FILE *fptr, struct png_chunk *chunk);
void png_interpretzTXt(void *data, uint32_t length);
void png_open(char filename[]);
int png_compareCRC(struct png_chunk *chunk);

#endif  // PNG_H
