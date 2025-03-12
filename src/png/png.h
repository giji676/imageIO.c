#ifndef PNG_H
#define PNG_H

#include <stdint.h>
#include <stdio.h>
#include "../image_common.h"

struct __attribute__((packed)) png_fileSignature {
    char signature[8];
};

void png_printFileSignature(struct png_fileSignature *fileSignature);
int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature);
void png_open(char filename[]);

#endif  // PNG_H
