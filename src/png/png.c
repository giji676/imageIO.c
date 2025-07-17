#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "./png.h"
#include "../crc/crc.h"

void png_printIHDR(struct png_IHDR *ihdr) {
    uint32_t width = __builtin_bswap32(ihdr->width);
    uint32_t height = __builtin_bswap32(ihdr->height);
    printf("width: %u\n", width);
    printf("height: %u\n", height);
    printf("bitDepth: %u\n", ihdr->bitDepth);
    printf("colorType: %u\n", ihdr->colorType);
    printf("compressionMethod: %u\n", ihdr->compressionMethod);
    printf("filterMethod: %u\n", ihdr->filterMethod);
    printf("interlaceMethod: %u", ihdr->interlaceMethod);
}

void png_printChunk(struct png_chunk *chunk) {
    printf("\n");
    printf("Chunk\n");
    printf("length: %u\n", chunk->length);
    printf("chunkType: %.4s\n", chunk->chunkType);
    if (strncmp(chunk->chunkType, "IHDR", 4) == 0 && chunk->length == 13) {
        png_printIHDR((struct png_IHDR *)chunk->chunkData);
    } else if (strncmp(chunk->chunkType, "zTXt", 4) == 0) {
        png_interpretzTXt(chunk->chunkData, chunk->length);
    } else {
        printf("chunkData:");
        for (uint32_t i = 0; i < chunk->length; ++i) {
            if (i % 16 == 0) {
                printf("\n");
            }
            printf("%02x ", ((unsigned char *)chunk->chunkData)[i]);
        }
    }
    printf("\n");
    printf("expected crc: 0x%08X\n", chunk->crc);
    int crc_compare_res = png_compareCRC(chunk);
    if (crc_compare_res == 1) {
        //printf("CRC MATCHING\n");
    } else {
        printf("CRC NOT MATCHING\n");
    }
}

int png_compareCRC(struct png_chunk *chunk) {
    int crc_inp_len = sizeof(chunk->chunkType) + (int)chunk->length;
    unsigned char *buff = malloc(crc_inp_len);
    if (buff == NULL) {
        printf("Failed to allocate memory for chunk crc buffer\n");
        return -1;
    }
    memcpy(buff, chunk->chunkType, sizeof(chunk->chunkType));
    memcpy(buff + sizeof(chunk->chunkType), chunk->chunkData, chunk->length);
    /*
    printf("raw buff:\n");
    for (int i = 0; i < crc_inp_len; i++) {
        printf("0x%02X ", buff[i]);
    }
    printf("\n");
    */
    
    unsigned long res = crc(buff, crc_inp_len);
    printf("calculated crc: 0x%lX\n", res);
    free(buff);
    if (res == chunk->crc) {
	return 1;
    } else {
	return 0;
    }
}

void png_interpretzTXt(void *data, uint32_t length) {
    if (data == NULL || length == 0) {
        printf("Invalid zTXt data\n");
        return;
    }

    unsigned char *bytes = (unsigned char *)data;
    struct png_zTXt ztxt;
    ztxt.keyword = (char *)bytes;

    uint32_t keyword_end = 0;

    while (keyword_end < length && bytes[keyword_end] != 0) {
        keyword_end++;
    }

    if (keyword_end >= length || keyword_end + 1 >= length) {
        printf("Invalid zTXt chunk format\n");
        return;
    }
    ztxt.compMethod = &bytes[keyword_end + 1];
    ztxt.compText = (char *)&bytes[keyword_end + 2];

    printf("Keyword: %s\n", ztxt.keyword);
    printf("Compression Method: %u\n", *ztxt.compMethod);

    uint32_t compText_length = length - keyword_end - 2;

    printf("Compressed Text:\n");

    for (uint32_t i = 0; i < compText_length; i++) {
        if (i > 0 && i % 16 == 0) {
            printf("\n");
        }
        printf("%02x ", (unsigned char)ztxt.compText[i]);
    }
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

int png_readFileSignature(FILE *fptr, struct png_fileSignature *fileSignature) {
    if (fread(fileSignature, sizeof(struct png_fileSignature), 1, fptr) != 1) {
        printf("Failed to read file signature\n");
        return -1;
    }
    return 1;
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
    if (chunk->length == 0) {
        chunk->chunkData = NULL;
    } else if (fread(chunk->chunkData, chunk->length, 1, fptr) != 1) {
        printf("Failed to read chunk data\n");
        free(chunk->chunkData);
        return -1;
    }

    if (strncmp(chunk->chunkType, "IHDR", 4) == 0 && chunk->length == 13) {
	/*
        ((struct png_IHDR *)chunk->chunkData)->width =
            __builtin_bswap32(((struct png_IHDR *)chunk->chunkData)->width);
        ((struct png_IHDR *)chunk->chunkData)->height =
            __builtin_bswap32(((struct png_IHDR *)chunk->chunkData)->height);
        */
    } else if (strncmp(chunk->chunkType, "zTXt", 4) == 0) {
        //png_interpretzTXt(chunk->chunkData, chunk->length);
    }

    if (fread(&chunk->crc, sizeof(chunk->crc), 1, fptr) != 1) {
        printf("Failed to read chunk crc\n");
        free(chunk->chunkData);
        return -1;
    }
    chunk->crc = __builtin_bswap32(chunk->crc);

    return 1;
}

int png_readChunks(FILE *fptr, struct png_chunk **chunks) {
    int chunkCount = 0;

    while (1) {
        if (chunkCount > 0) {
            size_t cSize = (chunkCount+1)*sizeof(struct png_chunk);
            struct png_chunk *temp = realloc(*chunks, cSize);
            if (temp == NULL) {
                printf("Failed to allocte memory for chunks\n");
                break;
            }
            *chunks = temp;
        }
        if (png_readChunk(fptr, &(*chunks)[chunkCount]) != 1) {
            printf("Error reading chunk or end of file\n");
            break;
        }
        png_printChunk(&(*chunks)[chunkCount]);

        if (strncmp((*chunks)[chunkCount].chunkType, "IEND", 4) == 0) {
            printf("\nEnd of file reached\n");
            chunkCount++;
            break;
        }
        chunkCount++;
    }

    return chunkCount;
}

void png_open(char filename[]) {
    FILE *fptr;

    make_crc_table();

    if ((fptr = fopen(filename, "rb")) == NULL) {
        printf("Failed to open file %s\n", filename);
        return;
    }

    struct png_fileSignature png_fileSignature;
    if (png_readFileSignature(fptr, &png_fileSignature) != 1) {
        return;
    }
    png_printFileSignature(&png_fileSignature);

    struct png_chunk *chunks = malloc(sizeof(struct png_chunk));
    if (chunks == NULL) {
        printf("Failed to allocte memory for chunks\n");
        return;
    }

    int chunkCount = png_readChunks(fptr, &chunks);
    if (chunkCount < 0) {
        printf("Error reading chunks\n");
        fclose(fptr);
        return;
    }
    fclose(fptr);

    for (int i = 0; i < chunkCount; ++i) {
        free(chunks[i].chunkData);
    }
    free(chunks);
}
