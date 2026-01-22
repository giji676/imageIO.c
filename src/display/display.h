#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> // for sleep

void show_raw_pixels(uint8_t *pixels, int width, int height, int bpp);
