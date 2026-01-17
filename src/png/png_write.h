#ifndef PNG_WRITE_H
#define PNG_WRITE_H

#include <stdint.h>
#include <stdio.h>
#include "../image_common.h"

struct huffmanCode {
    uint16_t code; // the bit pattern
    uint8_t length; // number of bits
};

int png_save(char filename[]);

#endif  // PNG_WRITE_H
