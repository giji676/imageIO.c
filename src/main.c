#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bmp/bmp.h"
#include "png/png.h"
#include "png/png_write.h"
#include "./display/display.h"

void printUsage() {
    printf("Usage: ./parser [OPTIONS] INPUT_FILE\n\n");
    printf("Arguments:\n");
    printf("  INPUT_FILE\tPath to the input file to parse (required)\n\n");
    printf("Options:\n");
    printf("  -d, --disp, --display\tDisplay the parsed image\n");
    printf("  -h, --help\tShow this help message and exit\n\n");
    printf("Examples:\n");
    printf("  ./parser image.png\n");
    printf("  ./parser --display image.png\n");
}

uint8_t *openImage(char *filename, uint32_t* width, uint32_t* height) {
    char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        printf("Error: No file extension found in \"%s\"\n", filename);
        return NULL;
    }

    if (strcasecmp(ext, ".bmp") == 0) {
        bmp_open(filename);
    } else if (strcasecmp(ext, ".png") == 0) {
        return png_open(filename, width, height);
    } else {
        printf("Error: Unsupported file format \"%s\"\n", ext);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    int display = 0;
    char *input_file = NULL;
    int save = 0;

    // Iterate over arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 ||
            strcmp(argv[i], "--disp") == 0 ||
            strcmp(argv[i], "--display") == 0) 
        {
            display = 1;
        } else if (strcmp(argv[i], "-s") == 0 ||
            strcmp(argv[i], "--save") == 0)
        {
            save = 1;
        } else if (argv[i][0] != '-') {
            // first non-flag argument is INPUT_FILE
            if (!input_file) {
                input_file = argv[i];
            } else {
                fprintf(stderr, "Unexpected argument: %s\n", argv[i]);
                printUsage();
                return 1;
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            printUsage();
            return 1;
        }
    }

    if (!input_file) {
        // input_file = "assets/example.png";
        fprintf(stderr, "Error: INPUT_FILE is required\n");
        printUsage();
        return 1;
    }

    uint32_t width, height;
    uint8_t *pixelData = openImage(input_file, &width, &height);
    if (pixelData == NULL) {
        printf("Error opening the image\n");
        return 1;
    }

    if (display) {
        show_raw_pixels(pixelData, width, height);
    }

    if (save) {
        png_save("output.png", pixelData, width, height, 3);
    }

    free(pixelData);

    return 0;
}
