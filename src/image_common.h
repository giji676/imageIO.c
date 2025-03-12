#ifndef IMAGE_COMMON_H
#define IMAGE_COMMON_H

#include <stdint.h>

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

#endif  // IMAGE_COMMON_H
