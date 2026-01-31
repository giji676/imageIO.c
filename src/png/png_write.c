#include "png_write.h"
#include "png.h"
#include "../crc/crc.h"
#include "../display/display.h"
#include "../log.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct huffmanCode fixed_huffman_code(uint16_t symbol) {
    struct huffmanCode hc;
    if (symbol <= 143) {
        hc.code = 0b00110000 + (symbol - 0);
        hc.length = 8;
    } else if (symbol <= 255) {
        hc.code = 0b110010000 + (symbol - 144);
        hc.length = 9;
    } else if (symbol <= 279) {
        hc.code = 0b0000000 + (symbol - 256);
        hc.length = 7;
    } else if (symbol <= 287) {
        hc.code = 0b11000000 + (symbol - 280);
        hc.length = 8;
    } else {
        hc.code = 0;
        hc.length = 0; // invalid
    }
    return hc;
}

void fixed_huffman(struct bitStream *bs, uint8_t *data, int size) {
    for (int i = 0; i < size; i++) {
        struct huffmanCode hc = fixed_huffman_code(data[i]);
        hc.code = reverse_bits(hc.code, hc.length);
        bitstream_write(bs, hc.length, hc.code);
    }

    struct huffmanCode eob = fixed_huffman_code(256);
    eob.code = reverse_bits(eob.code, eob.length);
    bitstream_write(bs, eob.length, eob.code);

    // Flush to next byte
    bitstream_flush(bs);
}

uint8_t *png_deflate(struct png_image *image,
                     struct png_IDAT *idat,
                     int *out_len)
{
    int bpp = 3;
    int row_bytes = image->ihdr.width * bpp;

    /* ---- Prepare filtered data ---- */
    idat->data_length = (row_bytes + 1) * image->ihdr.height;
    idat->data = malloc(idat->data_length);

    for (uint32_t i = 0; i < image->ihdr.height; i++) {
        int row_start = i * (row_bytes + 1);

        if (i == 0) {
            idat->data[row_start] = 0; // None
            memcpy(idat->data + row_start + 1,
                   image->pixels,
                   row_bytes);
        } else {
            idat->data[row_start] = 1; // Sub
            for (int x = 0; x < row_bytes; x++) {
                uint8_t raw  = image->pixels[i * row_bytes + x];
                uint8_t left = (x >= bpp)
                    ? image->pixels[i * row_bytes + x - bpp]
                    : 0;
                idat->data[row_start + 1 + x] = raw - left;
            }
        }
    }

    /* ---- Allocate output ---- */
    int max_out = idat->data_length + 6 + 5;
    uint8_t *out_buf = malloc(max_out);

    struct bitStream bs = {
        .data = out_buf,
        .length = max_out,
        .bytepos = 0
    };

    /* ---- ZLIB header (BYTE ALIGNED) ---- */
    idat->cm     = 8;
    idat->cinfo  = 0;
    idat->cmf    = (idat->cinfo << 4) | idat->cm;
    idat->flevel = 2;
    idat->fdict  = 0;
    idat->fcheck = 31 - ((idat->cmf << 8 |
                          (idat->flevel << 6)) % 31);
    idat->flg    = (idat->flevel << 6) |
                   (idat->fdict << 5) |
                   idat->fcheck;

    out_buf[bs.bytepos++] = idat->cmf;
    out_buf[bs.bytepos++] = idat->flg;

    /* ---- DEFLATE block header ---- */
    bitstream_write(&bs, 1, 1); // BFINAL
    bitstream_write(&bs, 2, 1); // BTYPE = fixed Huffman

    fixed_huffman(&bs, idat->data, idat->data_length);

    /* ---- Adler32 ---- */
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < idat->data_length; i++) {
        a = (a + idat->data[i]) % 65521;
        b = (b + a) % 65521;
    }
    free(idat->data);

    uint32_t adler32 = (b << 16) | a;

    bitstream_flush(&bs); // ensure byte alignment
    out_buf[bs.bytepos++] = (adler32 >> 24) & 0xFF;
    out_buf[bs.bytepos++] = (adler32 >> 16) & 0xFF;
    out_buf[bs.bytepos++] = (adler32 >>  8) & 0xFF;
    out_buf[bs.bytepos++] =  adler32        & 0xFF;

    *out_len = bs.bytepos;
    return out_buf;
}

uint8_t *serialize_ihdr(const struct png_IHDR *ihdr) {
    uint8_t *buf = malloc(sizeof(struct png_IHDR));
    if (!buf) return NULL;

    uint32_t w = __builtin_bswap32(ihdr->width);
    uint32_t h = __builtin_bswap32(ihdr->height);

    size_t off = 0;

    memcpy(buf + off, &w, sizeof(w)); off += sizeof(w);
    memcpy(buf + off, &h, sizeof(h)); off += sizeof(h);

    buf[off++] = ihdr->bitDepth;
    buf[off++] = ihdr->colorType;
    buf[off++] = ihdr->compressionMethod;
    buf[off++] = ihdr->filterMethod;
    buf[off++] = ihdr->interlaceMethod;

    return buf;
}

void write_chunk(FILE *fptr, struct png_chunk *chunk) {
    uint32_t chunkLength = __builtin_bswap32(chunk->length);
    uint32_t totalChunkSize = sizeof(chunk->length) + sizeof(chunk->chunkType) + chunk->length + sizeof(chunk->crc);
    uint8_t *buff = malloc(totalChunkSize);

    size_t off = 0;
    memcpy(buff + off, &chunkLength, sizeof(chunk->length)); off += sizeof(chunk->length);
    memcpy(buff + off, chunk->chunkType, sizeof(chunk->chunkType)); off += sizeof(chunk->chunkType);

    if (memcmp(chunk->chunkType, "IHDR", 4) == 0) {
        uint8_t *serializedData = serialize_ihdr((struct png_IHDR *)chunk->chunkData);
        memcpy(buff + off, serializedData, chunk->length); off += chunk->length;
        free(serializedData);
    } else {
        memcpy(buff + off, chunk->chunkData, chunk->length); off += chunk->length;
    }
    uint32_t crc_val = __builtin_bswap32(crc(buff + sizeof(chunk->length), sizeof(chunk->chunkType) + chunk->length));
    memcpy(buff + off, &crc_val, sizeof(chunk->crc));

    fwrite(buff, totalChunkSize, 1, fptr);
    free(buff);
}

int png_save(char filename[], uint8_t *data, uint32_t width, uint32_t height, uint8_t bpp) {
    FILE *fptr;
    if ((fptr = fopen(filename, "wb")) == NULL) {
        LOGE("Failed to open file %s for writing\n", filename);
        return -1;
    }

    struct png_fileSignature png_fileSignature = {
        .signature = {'\211','P','N','G','\r','\n','\032','\n'}
    };

    struct png_IHDR ihdr = {
        .width = width,
        .height = height,
        .bitDepth = 8,
        .colorType = 2,
        .compressionMethod = 0,
        .filterMethod = 0,
        .interlaceMethod = 0,
    };

    struct png_chunk ihdr_chunk = {
        .length = sizeof(struct png_IHDR),
        .chunkType = {'I','H','D','R'},
        .chunkData = &ihdr,
    };

    struct png_image image;
    image.ihdr = ihdr;

    image.pixels = data;
    image.pixel_size = ihdr.width * ihdr.height * bpp;

    struct png_IDAT idat;

    int idat_len;
    uint8_t *compressed = png_deflate(&image, &idat, &idat_len);

    struct png_chunk idat_chunk = {
        .length = idat_len,
        .chunkType = {'I','D','A','T'},
        .chunkData = compressed,
    };

    struct png_chunk iend_chunk = {
        .length = 0,
        .chunkType = {'I','E','N','D'},
        .chunkData = NULL,
    };

    fwrite(&png_fileSignature, sizeof(png_fileSignature), 1, fptr);
    write_chunk(fptr, &ihdr_chunk);
    write_chunk(fptr, &idat_chunk);
    write_chunk(fptr, &iend_chunk);
    free(compressed);
    // free(data);

    fclose(fptr);
    return 1;
}
