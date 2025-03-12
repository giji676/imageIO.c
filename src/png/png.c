#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "./png.h"

void png_open(char filename[]) {
    FILE *fptr;

    if ((fptr = fopen(filename, "rb")) == NULL) {
        printf("Failed to open file %s\n", filename);
        return;
    }
}
