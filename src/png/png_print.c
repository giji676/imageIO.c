#include "png_print.h"
#include "../log.h"

void png_printPixels(void *pixels, struct png_IHDR *ihdr, struct png_PLTE *plte) {
    for (uint32_t i = 0; i < ihdr->height; i++) {
        for (uint32_t j = 0; j < ihdr->width; j++) {
            switch (ihdr->colorType) {
                case 2:
                    if (ihdr->bitDepth == 8) {
                        struct rgbPixel *pixel = &((struct rgbPixel *)pixels)[i * ihdr->width + j];
                        LOGI_RAW("(%d,%d,%d)", pixel->r, pixel->g, pixel->b);
                    }
                    break;
                case 3:
                    if (ihdr->bitDepth == 8) {
                        uint8_t *indices = (uint8_t *)pixels;

                        uint8_t index = indices[i * ihdr->width + j];

                        if ((uint32_t)(index * 3 + 2) >= plte->length) {
                            LOGE("Palette index out of range: %u\n", index);
                            break;
                        }

                        uint8_t r = plte->data[index * 3 + 0];
                        uint8_t g = plte->data[index * 3 + 1];
                        uint8_t b = plte->data[index * 3 + 2];

                        LOGI_RAW("(%u,%u,%u)", r, g, b);
                    }
                    break;
            }
            LOGI_RAW(" ");
        }
        LOGI_RAW("\n");
    }
}

void png_printIDAT(struct png_IDAT *idat) {
    LOGI("CMF: 0x%02X\n", idat->cmf);
    LOGI("CM: %u\n", idat->cm);
    LOGI("CINFO: %u\n", idat->cinfo);
    LOGI("FLG: 0x%02X\n", idat->flg);
    LOGI("FCHECK: %u\n", idat->fcheck);
    LOGI("FDICT: %u\n", idat->fdict);
    LOGI("FLEVEL: %u\n", idat->flevel);
    LOGI("DATA_LENGTH: %u\n", idat->data_length);
    LOGI("DATA: ...\n");
    // for (uint32_t i = 0; i < idat->data_length; ++i) {
    //     LOGI("%02x ", idat->data[i]);
    //     if (i > 0 && i % 16 == 15) {
    //         LOGI("\n");
    //     }
    // }
    // printf("\n");
    LOGI("ADLER32: 0x%08X\n", idat->adler32);
}

void png_printIHDR(struct png_IHDR *ihdr) {
    uint32_t width = __builtin_bswap32(ihdr->width);
    uint32_t height = __builtin_bswap32(ihdr->height);
    LOGI("width: %u\n", width);
    LOGI("height: %u\n", height);
    LOGI("bitDepth: %u\n", ihdr->bitDepth);
    LOGI("colorType: %u\n", ihdr->colorType);
    LOGI("compressionMethod: %u\n", ihdr->compressionMethod);
    LOGI("filterMethod: %u\n", ihdr->filterMethod);
    LOGI("interlaceMethod: %u\n", ihdr->interlaceMethod);
}

void png_printFileSignature(struct png_fileSignature *fileSignature) {
    LOGI("\n");
    LOGI("File Signature\n");
    LOGI("signature: ");
    for (int i = 0; i < (int)sizeof(fileSignature->signature); ++i) {
        LOGI_RAW("%02x ", (unsigned char)fileSignature->signature[i]);
    }
    LOGI_RAW("\n");
}

