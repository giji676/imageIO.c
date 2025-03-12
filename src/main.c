#include <stdio.h>
#include <string.h>
#include "bmp/bmp.h"
#include "png/png.h"

void processFile(char *filename) {
    char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        printf("Error: No file extension found in \"%s\"\n", filename);
        return;
    }

    if (strcasecmp(ext, ".bmp") == 0) {
        bmp_open(filename);
    } else if (strcasecmp(ext, ".png") == 0) {
        png_open(filename);
    } else {
        printf("Error: Unsupported file format \"%s\"\n", ext);
    }
}

int main() {
    char filename[] = "assets/bmpExample.bmp";
    processFile(filename);

    return 0;
}
