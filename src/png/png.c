#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "./png.h"

void png_printChunkLayout(struct png_chunkLayout *chunkLayout) {
    printf("length: %u\n", chunkLayout->length);
    printf("chunkType: %.4s\n", chunkLayout->chunkType);
    printf("chunkData: ");
    for (int i = 0; i < (int)sizeof(chunkLayout->chunkData); ++i) {
        printf("%d ", chunkLayout->chunkData[i]);
    }
    printf("\n");
    printf("crc: %u\n", chunkLayout->crc);
}

void png_printFileSignature(struct png_fileSignature *fileSignature) {
    printf("signature: ");
    for (int i = 0; i < (int)sizeof(fileSignature->signature); ++i) {
        printf("%02x ", (unsigned char)fileSignature->signature[i]);
    }
    printf("\n");
}

int png_readChunkLayout(FILE *fptr, struct png_chunkLayout *chunkLayout) {
    if (fread(chunkLayout,
              (sizeof(chunkLayout->length) + sizeof(chunkLayout->chunkType)),
              1, fptr) != 1) {
        printf("Failed to read chunk layout\n");
        return -1;
    }

    chunkLayout->length = __builtin_bswap32(chunkLayout->length);
    chunkLayout->chunkData = (uint8_t *)malloc(chunkLayout->length);
    if (chunkLayout->chunkData == NULL) {
        printf("Failed to allocate memory for chunk data\n");
        return -1;
    }

    if (fread(chunkLayout->chunkData, chunkLayout->length, 1, fptr) != 1) {
        printf("Failed to read chunk data\n");
        free(chunkLayout->chunkData);
        return -1;
    }

    if (fread(chunkLayout, sizeof(chunkLayout->crc), 1, fptr) != 1) {
        printf("Failed to read chunk crc\n");
        free(chunkLayout->chunkData);
        return -1;
    }
    chunkLayout->crc = __builtin_bswap32(chunkLayout->crc);
    return 1;
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
    printf("File Signature\n");
    png_printFileSignature(&png_fileSignature);

    struct png_chunkLayout png_chunkLayout;
    if (png_readChunkLayout(fptr, &png_chunkLayout) != 1) {
        return;
    }
    printf("\n");
    printf("Chunk Layout\n");
    png_printChunkLayout(&png_chunkLayout);
    fclose(fptr);
}
