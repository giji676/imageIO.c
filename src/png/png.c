#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "./png.h"

void png_printIHDR(struct png_IHDR *ihdr) {
    printf("\n");
    printf("IHDR\n");
    printf("width: %u\n", ihdr->width);
    printf("height: %u\n", ihdr->height);
    printf("bitDepth: %u\n", ihdr->bitDepth);
    printf("colorType: %u\n", ihdr->colorType);
    printf("compressionMethod: %u\n", ihdr->compressionMethod);
    printf("filterMethod: %u\n", ihdr->filterMethod);
    printf("interlaceMethod: %u\n", ihdr->interlaceMethod);
}

void png_printChunk(struct png_chunk *chunk) {
    printf("\n");
    printf("Chunk\n");
    printf("length: %u\n", chunk->length);
    printf("chunkType: %.4s\n", chunk->chunkType);
    if (strncmp(chunk->chunkType, "IHDR", 4) == 0 && chunk->length == 13) {
        png_printIHDR((struct png_IHDR *)chunk->chunkData);
    } else {
        printf("chunkData:\n");
        for (uint32_t i = 0; i < chunk->length; ++i) {
            if (i % 16 == 0) {  // Add a newline every 16 bytes for better readability
                printf("\n");
            }
            printf("%02x ", ((unsigned char *)chunk->chunkData)[i]);
        }
    }
    printf("\n");
    printf("crc (Hex): %08X\n", chunk->crc);
}

void png_printFileSignature(struct png_fileSignature *fileSignature) {
    printf("\n");
    printf("File Signature\n");
    printf("signature: ");
    for (int i = 0; i < (int)sizeof(fileSignature->signature); ++i) {
        printf("%02x ", (unsigned char)fileSignature->signature[i]);
    }
    printf("\n");
}

int png_readChunk(FILE *fptr, struct png_chunk *chunk) {
    if (fread(chunk,
              (sizeof(chunk->length) + sizeof(chunk->chunkType)),
              1, fptr) != 1) {
        printf("Failed to read chunk layout\n");
        return -1;
    }

    chunk->length = __builtin_bswap32(chunk->length);
    chunk->chunkData = (void *)malloc(chunk->length);
    if (chunk->chunkData == NULL) {
        printf("Failed to allocate memory for chunk data\n");
        return -1;
    }

    if (fread(chunk->chunkData, chunk->length, 1, fptr) != 1) {
        printf("Failed to read chunk data\n");
        free(chunk->chunkData);
        return -1;
    }

    if (strncmp(chunk->chunkType, "IHDR", 4) == 0 && chunk->length == 13) {
        ((struct png_IHDR *)chunk->chunkData)->width =
            __builtin_bswap32(((struct png_IHDR *)chunk->chunkData)->width);
        ((struct png_IHDR *)chunk->chunkData)->height =
            __builtin_bswap32(((struct png_IHDR *)chunk->chunkData)->height);
    }

    if (fread(&chunk->crc, sizeof(chunk->crc), 1, fptr) != 1) {
        printf("Failed to read chunk crc\n");
        free(chunk->chunkData);
        return -1;
    }
    chunk->crc = __builtin_bswap32(chunk->crc);

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
    png_printFileSignature(&png_fileSignature);

    struct png_chunk png_chunk0;
    if (png_readChunk(fptr, &png_chunk0) != 1) {
        return;
    }
    png_printChunk(&png_chunk0);

    struct png_chunk png_chunk1;
    if (png_readChunk(fptr, &png_chunk1) != 1) {
        return;
    }
    png_printChunk(&png_chunk1);

    fclose(fptr);
    free(png_chunk0.chunkData);
}
