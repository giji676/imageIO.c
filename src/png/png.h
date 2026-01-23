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

// For combining all the IDAT chunks
struct png_IDAT_stream {
    uint8_t *data;
    size_t length;
};

struct png_zTXt {
    char *keyword;
    uint8_t *compMethod;
    char *compText;
};

struct png_PLTE {
    uint32_t length;
    uint8_t *data;
};

struct png_tRNS {
    uint8_t *alpha;
    uint32_t length;
};

struct png_image {
    struct png_IHDR ihdr;
    struct png_PLTE plte;
    struct png_tRNS trns;
    struct png_IDAT_stream idat_stream;
    uint8_t *pixels;
    size_t pixel_size; // total size of pixel data in bytes
};

struct output_image {
    uint8_t *pixels;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
};

struct output_image *png_open(char filename[]);

#endif  // PNG_H
