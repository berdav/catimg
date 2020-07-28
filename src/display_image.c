#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "sh_image.h"
#include <unistd.h>

// Transparency threshold -- all pixels with alpha below 25%.
#define TRANSP_ALPHA 64

volatile int loops = -1, loop = -1;
volatile char stop = 0;

static uint32_t pixelToInt(const color_t *pixel) {
    if (pixel->a == 0)
        return 0xffff;
    else if (pixel->r == pixel->g && pixel->g == pixel->b)
        return 232 + (pixel->r * 23)/255;
    else
        return (16 + ((pixel->r*5)/255)*36
                + ((pixel->g*5)/255)*6
                + (pixel->b*5)/255);
}

void display_image(FILE *h, image_t *img, int true_color, int precision) {
    // Save the cursor position and hide it
    fprintf(h, "\e[s\e[?25l");
    while (loop++ < loops || loops < 0) {
        uint32_t offset = 0;
        for (uint32_t frame = 0; frame < img->frames; frame++) {
            if (frame > 0 || loop > 0) {
                if (frame > 0)
                    usleep(img->delays[frame - 1] * 10000);
                else
                    usleep(img->delays[img->frames - 1] * 10000);
                fprintf(h, "\e[u");
            }
            uint32_t index, x, y;

            for (y = 0; y < img->height; y += precision) {
                for (x = 0; x < img->width; x++) {
                    index = y * img->width + x + offset;
                    const color_t* upperPixel = &img->pixels[index];
                    uint32_t fgCol = pixelToInt(upperPixel);
                    if (precision == 2) {
                        if (y < img->height - 1) {
                            const color_t* lowerPixel = &img->pixels[index + img->width];
                            uint32_t bgCol = pixelToInt(&img->pixels[index + img->width]);
                            if (upperPixel->a < TRANSP_ALPHA) { // first pixel is transparent
                                if (lowerPixel->a < TRANSP_ALPHA)
                                    fprintf(h, "\e[m ");
                                else
                                    if (true_color)
                                        fprintf(h, "\x1b[0;38;2;%d;%d;%dm\u2584",
                                               lowerPixel->r, lowerPixel->g, lowerPixel->b);
                                    else
                                        fprintf(h, "\e[0;38;5;%um\u2584", bgCol);
                            } else {
                                if (lowerPixel->a < TRANSP_ALPHA)
                                    if (true_color)
                                        fprintf(h, "\x1b[0;38;2;%d;%d;%dm\u2580",
                                               upperPixel->r, upperPixel->g, upperPixel->b);
                                    else
                                        fprintf(h, "\e[0;38;5;%um\u2580", fgCol);
                                else
                                    if (true_color)
                                        fprintf(h, "\x1b[48;2;%d;%d;%dm\x1b[38;2;%d;%d;%dm\u2584",
                                               upperPixel->r, upperPixel->g, upperPixel->b,
                                               lowerPixel->r, lowerPixel->g, lowerPixel->b);
                                    else
                                         fprintf(h, "\e[38;5;%u;48;5;%um\u2580", fgCol, bgCol);
                            }
                        } else { // this is the last line
                            if (upperPixel->a < TRANSP_ALPHA)
                                fprintf(h, "\e[m ");
                            else
                                if (true_color)
                                    fprintf(h, "\x1b[0;38;2;%d;%d;%dm\u2580",
                                           upperPixel->r, upperPixel->g, upperPixel->b);
                                else
                                    fprintf(h, "\e[38;5;%um\u2580", fgCol);
                        }
                    } else {
                        if (img->pixels[index].a < TRANSP_ALPHA)
                            fprintf(h, "\e[m  ");
                        else
                            if (true_color)
                                fprintf(h, "\x1b[0;48;2;%d;%d;%dm  ",
                                       img->pixels[index].r, img->pixels[index].g, img->pixels[index].b);
                            else
                                fprintf(h, "\e[48;5;%um  ", fgCol);
                    }
                }
                fprintf(h, "\e[m\n");
            }
            offset += img->width * img->height;
            if (stop) frame = img->frames;
        }
    }
    // Display the cursor again
    fprintf(h, "\e[?25h");
}
