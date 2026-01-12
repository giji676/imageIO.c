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

struct png_IDAT {
    uint8_t cmf;
    uint8_t flg;
    uint8_t cm;
    uint8_t cinfo;
    uint8_t fcheck;
    uint8_t fdict;
    uint8_t flevel;
    uint32_t data_length;
    uint8_t *data;
    uint32_t adler32;
};

struct png_zTXt {
    char *keyword;
    uint8_t *compMethod;
    char *compText;
};

struct png_image {
    struct png_IHDR ihdr;
    uint8_t *pixels;
    size_t pixel_size;
};

int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature);
int png_readChunks(FILE *fptr, struct png_chunk **chunk, struct png_image *image);
int png_readChunk(FILE *fptr, struct png_chunk *chunk);
int png_readIDAT(void *data, uint32_t length, struct png_IDAT *idat);

void png_printIHDR(struct png_IHDR *ihdr);
uint8_t *png_processIDAT(void *data, uint32_t length,
                         struct png_IHDR *ihdr,
                         size_t *out_size);
void png_printzTXt(void *data);
void png_printFileSignature(struct png_fileSignature *fileSignature);
void png_printChunk(struct png_chunk *chunk, struct png_image *image);
void png_printIDAT(struct png_IDAT *idat);
void png_printPixels(void *pixels, struct png_IHDR *ihdr);

int png_fixedHuffmanDecode(struct bitStream *ds,
                           uint8_t *output,
                           size_t *output_pos,
                           uint32_t expected);
int png_decodeFixedHuffmanSymbol(struct bitStream *ds, uint32_t *symbol);
void png_interpretzTXt(void *data, uint32_t length);
void png_open(char filename[]);
int png_compareCRC(struct png_chunk *chunk);
int png_compareAdler32(struct png_IDAT *idat, uint8_t *output, size_t output_pos);


#endif  // PNG_H
