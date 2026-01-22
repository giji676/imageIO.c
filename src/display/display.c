#include "./display.h"

// bpp = 3 (RGB) or 4 (RGBA)
void show_raw_pixels(uint8_t *pixels, int width, int height, int bpp) {
    int scale = 1000 / height;
    if (scale < 1) scale = 1;

    uint8_t *scaled_data = malloc(width * scale * height * scale * 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r = pixels[(y*width + x)*bpp + 0];
            uint8_t g = pixels[(y*width + x)*bpp + 1];
            uint8_t b = pixels[(y*width + x)*bpp + 2];
            uint8_t a = (bpp == 4) ? pixels[(y*width + x)*bpp + 3] : 255;

            // simple alpha over black
            r = (r * a) / 255;
            g = (g * a) / 255;
            b = (b * a) / 255;

            for (int dy = 0; dy < scale; dy++) {
                for (int dx = 0; dx < scale; dx++) {
                    int sx = x*scale + dx;
                    int sy = y*scale + dy;
                    int off = (sy*(width*scale) + sx) * 4;

                    scaled_data[off + 0] = b;
                    scaled_data[off + 1] = g;
                    scaled_data[off + 2] = r;
                    scaled_data[off + 3] = 0;
                }
            }
        }
    }

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return;

    int screen = DefaultScreen(dpy);
    Window win = XCreateSimpleWindow(
        dpy, RootWindow(dpy, screen),
        0, 0, width*scale, height*scale, 1,
        BlackPixel(dpy, screen),
        WhitePixel(dpy, screen)
    );

    XStoreName(dpy, win, "PNG Display");
    XSelectInput(dpy, win, ExposureMask | KeyPressMask);
    XMapWindow(dpy, win);

    GC gc = DefaultGC(dpy, screen);

    XEvent e;
    do { XNextEvent(dpy, &e); } while (e.type != Expose);

    int depth = DefaultDepth(dpy, screen);

    XImage *img = XCreateImage(
        dpy,
        DefaultVisual(dpy, screen),
        depth,
        ZPixmap,
        0,
        (char *)scaled_data,
        width*scale,
        height*scale,
        32,
        0
    );

    XPutImage(dpy, win, gc, img, 0, 0, 0, 0, width*scale, height*scale);
    XFlush(dpy);

    XNextEvent(dpy, &e);

    XDestroyImage(img);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}
