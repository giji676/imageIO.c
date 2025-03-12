#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "./png.h"


void png_printFileSignature(struct png_fileSignature *fileSignature) {
    printf("signature: ");
    for (int i = 0; i < (int)sizeof(fileSignature->signature); ++i) {
        printf("%02x ", (unsigned char)fileSignature->signature[i]);
    }
    printf("\n");
}

int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature) {
    if (fread(fileSignature, sizeof(struct png_fileSignature), 1, fptr) != 1) {
        printf("Failed to read file signature\n");
        return -1;
    }
    return 1;
}

void png_open(char filename[]) {
    FILE *fptr;

    if ((fptr = fopen(filename, "rb")) == NULL) {
        printf("Failed to open file %s\n", filename);
        return;
    }

    struct png_fileSignature png_fileSignature;
    if (png_readFileSignature(fptr, &png_fileSignature) != 1) {
        return;
    }
    printf("\n");
    printf("File Header\n");
    png_printFileSignature(&png_fileSignature);
}
