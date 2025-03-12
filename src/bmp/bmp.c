#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "./bmp.h"

void bmp_printFileHeader(struct fileHeader *header) {
    printf("signature: %c%c\n", header->signature[0],
                                header->signature[1]);
    printf("size: %d\n", header->size);
    printf("reserve1: %d\n", header->reserve1);
    printf("reserve2: %d\n", header->reserve2);
    printf("offset: %d\n", header->offset);
}

void bmp_printBitmapInfoHeader(struct bitmapInfoHeader *header) {
    printf("size: %d\n", header->size);
    printf("width: %d\n", header->width);
    printf("height: %d\n", header->height);
    printf("planes: %d\n", header->planes);
    printf("bitCount: %d\n", header->bitCount);
    printf("compression: %d\n", header->compression);
    printf("sizeImage: %d\n", header->sizeImage);
    printf("horizontalRes: %d\n", header->horizontalRes);
    printf("verticalRes: %d\n", header->verticalRes);
    printf("colorsUsed: %d\n", header->colorsUsed);
    printf("colorsImportant: %d\n", header->colorsImportant);
}

void bmp_printPixels(void *pixels,
                 struct bitmapInfoHeader *header) {
    for (int i = 0; i < header->height; ++i) {
        for (int j = 0; j < header->width; ++j) {
            if (header->bitCount == 24) {
                struct rgbPixel *pixel = &((struct rgbPixel *)pixels)[i * header->width + j];
                printf("(%d,%d,%d)", pixel->r, pixel->g, pixel->b);
            } else if (header->bitCount == 32) {
                struct rgbaPixel *pixel = &((struct rgbaPixel *)pixels)[i * header->width + j];
                printf("(%d,%d,%d,%d)", pixel->r, pixel->g, pixel->b, pixel->a);
            }
            printf(" ");
        }
        printf("\n");
    }
}

int bmp_readFileHeader(FILE *fptr, struct fileHeader *header) {
    if (fread(header, sizeof(struct fileHeader), 1, fptr) != 1) {
        printf("Failed to read file header\n");
        return -1;
    }
    return 1;
}

int bmp_readBitmapInfoHeader(FILE *fptr, struct bitmapInfoHeader *header) {
    if (fread(header, sizeof(struct bitmapInfoHeader), 1, fptr) != 1) {
        printf("Failed to read bitmap info header\n");
        return -1;
    }
    return 1;
}

void* bmp_mallocPixels(int width, int height, size_t size) {
    void *pixelPtr = malloc(height * width * size);
    if (pixelPtr == NULL) {
        printf("Failed to allocate memory for pixels\n");
        return NULL;
    }
    return pixelPtr;
}

void* bmp_readPixels(FILE *fptr,
                struct bitmapInfoHeader *header) {
    int rowSize = ((header->bitCount *
                    header->width + 31) / 32) * 4;
    printf("Row size: %d\n", rowSize);
    switch (header->bitCount) {
        case 24:
            break;
        case 32:
            break;
        default:
            printf("BitCount of %d not yet supported!\n", header->bitCount);
            return NULL;
    }
    void *pixels;
    if (header->bitCount == 24) {
        pixels = bmp_mallocPixels(
            header->width,
            header->height,
            sizeof(struct rgbPixel));
    } else if (header->bitCount == 32) {
        pixels = bmp_mallocPixels(
            header->width,
            header->height,
            sizeof(struct rgbaPixel));
    }

    if (pixels == NULL) {
        return NULL;
    }

    unsigned char *rowData = (unsigned char *)malloc(rowSize);
    if (rowData == NULL) {
        printf("Failed to allocate memory for row buffer\n");
        free(pixels);
        return NULL;
    }

    for (int i = 0; i < header->height; ++i) {
        fread(rowData, rowSize, 1, fptr);
        for (int j = 0; j < header->width; ++j) {
            int srcIndex = j * (header->bitCount / 8);
            int dstIndex = (header->height-1-i) * header->width + j;

            if (header->bitCount == 24) {
                struct rgbPixel *pixel = &((struct rgbPixel *)pixels)[dstIndex];
                pixel->b = rowData[srcIndex+0];
                pixel->g = rowData[srcIndex+1];
                pixel->r = rowData[srcIndex+2];
            } else if (header->bitCount == 32) {
                struct rgbaPixel *pixel = &((struct rgbaPixel *)pixels)[dstIndex];
                pixel->b = rowData[srcIndex+0];
                pixel->g = rowData[srcIndex+1];
                pixel->r = rowData[srcIndex+2];
                pixel->a = rowData[srcIndex+3];
            }
        }
    }
    free(rowData);
    return pixels;
}

void bmp_open(char filename[]) {
    FILE *fptr;

    if ((fptr = fopen(filename, "rb")) == NULL) {
        printf("Failed to bmp_open file %s\n", filename);
        return;
    }

    struct fileHeader fileHeader;
    if (bmp_readFileHeader(fptr, &fileHeader) != 1) {
        return;
    }
    printf("\n");
    printf("File Header\n");
    bmp_printFileHeader(&fileHeader);

    fseek(fptr, sizeof(struct fileHeader), SEEK_SET);

    struct bitmapInfoHeader bitmapInfoHeader;
    if (bmp_readBitmapInfoHeader(fptr, &bitmapInfoHeader) != 1) {
        return;
    }
    printf("\n");
    printf("Bitmap Info Header\n");
    bmp_printBitmapInfoHeader(&bitmapInfoHeader);

    fseek(fptr, fileHeader.offset, SEEK_SET);
    void *pixels = bmp_readPixels(fptr, &bitmapInfoHeader);
    if (pixels == NULL) {
        return;
    }
    printf("\n");
    printf("Pixels\n");
    bmp_printPixels(pixels, &bitmapInfoHeader);

    fclose(fptr);
    free(pixels);
}
