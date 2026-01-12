#include <stdio.h>
#include <string.h>
#include "bmp/bmp.h"
#include "png/png.h"

void print_usage() {
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

void processFile(char *filename, int display) {
    char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        printf("Error: No file extension found in \"%s\"\n", filename);
        return;
    }

    if (strcasecmp(ext, ".bmp") == 0) {
        bmp_open(filename);
    } else if (strcasecmp(ext, ".png") == 0) {
        png_open(filename, display);
    } else {
        printf("Error: Unsupported file format \"%s\"\n", ext);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    int display = 0;
    char *input_file = NULL;

    // Iterate over arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 ||
            strcmp(argv[i], "--disp") == 0 ||
            strcmp(argv[i], "--display") == 0) 
        {
            display = 1;
        } else if (argv[i][0] != '-') {
            // first non-flag argument is INPUT_FILE
            if (!input_file) {
                input_file = argv[i];
            } else {
                fprintf(stderr, "Unexpected argument: %s\n", argv[i]);
                print_usage();
                return 1;
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage();
            return 1;
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: INPUT_FILE is required\n");
        print_usage();
        return 1;
    }

    processFile(input_file, display);

    return 0;
}
