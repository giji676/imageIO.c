#include "./display.h"

// Example function to show raw RGB pixels
void show_raw_pixels(uint8_t *pixels, int width, int height) {
    int scale = 500;
    uint8_t *scaled_data = malloc(width * scale * height * scale * 4);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r = pixels[(y*width + x)*3 + 0];
            uint8_t g = pixels[(y*width + x)*3 + 1];
            uint8_t b = pixels[(y*width + x)*3 + 2];

            for (int dy = 0; dy < scale; dy++) {
                for (int dx = 0; dx < scale; dx++) {
                    int sx = x*scale + dx;
                    int sy = y*scale + dy;
                    scaled_data[(sy*(width*scale) + sx)*4 + 0] = b;
                    scaled_data[(sy*(width*scale) + sx)*4 + 1] = g;
                    scaled_data[(sy*(width*scale) + sx)*4 + 2] = r;
                    scaled_data[(sy*(width*scale) + sx)*4 + 3] = 0;
                }
            }
        }
    }

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open X display\n");
        return;
    }

    int screen = DefaultScreen(dpy);
    Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen),
                                     0, 0, width*scale, height*scale, 1,
                                     BlackPixel(dpy, screen),
                                     WhitePixel(dpy, screen));
    XStoreName(dpy, win, "Raw PNG Display");
    XSelectInput(dpy, win, ExposureMask | KeyPressMask);
    XMapWindow(dpy, win);

    GC gc = DefaultGC(dpy, screen);

    // Wait for the window to be ready
    XEvent e;
    do {
        XNextEvent(dpy, &e);
    } while (e.type != Expose);

    // XImage expects pixel data in 32-bit format (ARGB or similar)
    // We need to convert RGB to 32-bit
    int depth = DefaultDepth(dpy, screen);
    if (depth != 24 && depth != 32) {
        fprintf(stderr, "Warning: unexpected screen depth %d\n", depth);
    }

    int bytes_per_pixel = 4; // assuming 32-bit visual
    uint8_t *image_data = malloc(width * height * bytes_per_pixel);
    if (!image_data) {
        fprintf(stderr, "Failed to allocate image buffer\n");
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return;
    }

    for (int i = 0; i < width * height; i++) {
        image_data[i * 4 + 0] = pixels[i * 3 + 2]; // Blue
        image_data[i * 4 + 1] = pixels[i * 3 + 1]; // Green
        image_data[i * 4 + 2] = pixels[i * 3 + 0]; // Red
        image_data[i * 4 + 3] = 0;                 // Alpha (unused)
    }

    XImage *img = XCreateImage(dpy, DefaultVisual(dpy, screen),
                               depth, ZPixmap, 0,
                               (char *)scaled_data,
                               width*scale, height*scale,
                               32, 0);

    XPutImage(dpy, win, gc, img, 0, 0, 0, 0, width*scale, height*scale);
    XFlush(dpy);

    printf("Press any key in the window to exit...\n");
    XNextEvent(dpy, &e); // wait for key press

    XDestroyImage(img);   // frees image_data too
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}
