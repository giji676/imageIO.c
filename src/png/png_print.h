#ifndef PNG_PRINT_H
#define PNG_PRINT_H

#include "png.h"

void png_printPixels(void *pixels, struct png_IHDR *ihdr, struct png_PLTE *plte);
void png_printIDAT(struct png_IDAT *idat);
void png_printIHDR(struct png_IHDR *ihdr);
void png_printFileSignature(struct png_fileSignature *fileSignature);

#endif  // PNG_PRINT_H
