#ifndef IMAGE_COMMON_H
#define IMAGE_COMMON_H

#include <stdint.h>
#include <stddef.h>

struct __attribute__((packed)) rgbPixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct __attribute__((packed)) rgbaPixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct bitStream {
    uint8_t *data;   // pointer to the byte buffer
    size_t bitpos;   // current bit position in the stream
    size_t length;   // total length of data in bytes
};

void bitstream_init(struct bitStream *bs, uint8_t *data, size_t length);
int bitstream_read(struct bitStream *bs, int n, uint32_t *out);
int bitstream_peek(struct bitStream *bs, int n, uint32_t *out);
void bitstream_align_byte(struct bitStream *bs);

#endif  // IMAGE_COMMON_H
