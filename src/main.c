#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bmp/bmp.h"
#include "png/png.h"
#include "png/png_write.h"
#include "display/display.h"
#include "log.h"

int g_log_level = LOG_WARN;

void printUsage() {
    printf("Usage: ./parser [OPTIONS] INPUT_FILE\n\n");
    printf("Arguments:\n");
    printf("  INPUT_FILE\tPath to the input file to parse (required)\n\n");
    printf("Options:\n");
    printf("  -d, --disp, --display\tDisplay the parsed image\n");
    printf("  -s, --save\tSave the raw pixels back to a png file\n");
    printf("  --log=0|1|2\tSpecify log level (0=ERROR, 1=WARNING, 2=INFO)\n");
    printf("  -h, --help\tShow this help message and exit\n\n");
    printf("Examples:\n");
    printf("  ./parser image.png\n");
    printf("  ./parser --display image.png\n");
}

struct output_image *openImage(char *filename) {
    char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        printf("Error: No file extension found in \"%s\"\n", filename);
        return NULL;
    }

    if (strcasecmp(ext, ".bmp") == 0) {
        bmp_open(filename);
    } else if (strcasecmp(ext, ".png") == 0) {
        return png_open(filename);
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

    char *input_file = NULL;
    int display = 0;
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
        } else if (strncmp(argv[i], "--log=", 6) == 0)
        {
            g_log_level = atoi(argv[i] + 6);
            if (g_log_level < 0 || g_log_level > 2) {
                fprintf(stderr, "Invalid log level: %d\n", g_log_level);
                return 1;
            }
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
        fprintf(stderr, "Error: INPUT_FILE is required\n");
        printUsage();
        return 1;
    }

    struct output_image *image = openImage(input_file);
    if (image == NULL) {
        printf("Error opening the image\n");
        return 1;
    }

    if (display) {
        show_raw_pixels(image->pixels, image->width, image->height, image->bpp);
    }

    if (save) {
        png_save("output.png", image->pixels, image->width, image->height, image->bpp);
    }

    free(image->pixels);
    free(image);

    return 0;
}
