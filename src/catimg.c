#include "display_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sh_image.h"
#include "sh_utils.h"
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>

#define USAGE "Usage: catimg [-hct] [-w width | -H height] [-l loops] [-r resolution] image-file\n\n" \
  "  -h: Displays this message\n"                                      \
  "  -w: Terminal width/columns by default\n"                           \
  "  -H: Terminal height/row by default\n"                           \
  "  -l: Loops are only useful with GIF files. A value of 1 means that the GIF will " \
  "be displayed twice because it loops once. A negative value means infinite " \
  "looping\n"                                                           \
  "  -r: Resolution must be 1 or 2. By default catimg checks for unicode support to " \
  "use higher resolution\n" \
  "  -c: Convert colors to a restricted palette\n" \
  "  -t: Disables true color (24-bit) support, falling back to 256 color\n"

#define ERR_WIDTH_OR_HEIGHT "[ERROR] '-w' and '-H' can't be used at the same time\n\n"

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

// For <C-C>
extern int loops, loop;
extern char stop;

void intHandler() {
    loops = loop;
    stop = 1;
}

int getopt(int argc, char * const argv[], const char *optstring);

char supportsUTF8() {
    const char* LC_ALL = getenv("LC_ALL");
    const char* LANG = getenv("LANG");
    const char* LC_CTYPE = getenv("LC_CTYPE");
    const char* UTF = "UTF-8";
    return (LC_ALL && strstr(LC_ALL, UTF))
        || (LANG && strstr(LANG, UTF))
        || (LC_CTYPE && strstr(LC_CTYPE, UTF));
}

int main(int argc, char *argv[])
{
    init_hash_colors();
    image_t img;
    char *file;
    char *num;
    int c;
    opterr = 0;

    uint32_t cols = 0, rows = 0, precision = 0;
    uint32_t max_cols = 0, max_rows = 0;
    uint8_t convert = 0;
    uint8_t true_color = 1;
    uint8_t adjust_to_height = 0, adjust_to_width = 0;
    float scale_cols = 0, scale_rows = 0;

    while ((c = getopt (argc, argv, "H:w:l:r:hct")) != -1)
        switch (c) {
            case 'H':
                rows = strtol(optarg, &num, 0) >> 1;
                if (adjust_to_width) {
                    // only either adjust to width or adjust to height is allowed, but not both.
                    fprintf(stderr, ERR_WIDTH_OR_HEIGHT);
                    fprintf(stderr, USAGE);
                    exit(1);
                }
                adjust_to_height = 1;
                break;
            case 'w':
                cols = strtol(optarg, &num, 0) >> 1;
                if (adjust_to_height) {
                    // only either adjust to width or adjust to height is allowed, but not both.
                    fprintf(stderr, ERR_WIDTH_OR_HEIGHT);
                    fprintf(stderr, USAGE);
                    exit(1);
                }
                adjust_to_width = 1;
                break;
            case 'l':
                loops = strtol(optarg, &num, 0);
                break;
            case 'r':
                precision = strtol(optarg, &num, 0);
                break;
            case 'h':
                fprintf(stderr, USAGE);
                exit(0);
                break;
            case 'c':
                convert = 1;
                break;
            case 't':
                true_color = 0;
                break;
            default:
                fprintf(stderr, USAGE);
                exit(1);
                break;
        }

    if (argc > 1)
        file = argv[argc-1];
    else {
        fprintf(stderr, USAGE);
        exit(1);
    }

    if (precision == 0 || precision > 2) {
        if (supportsUTF8())
            precision = 2;
        else
            precision = 1;
    }

    // if precision is 2 we can use the terminal full width/height. Otherwise we can only use half
    max_cols = terminal_columns() / (2 / precision);
    max_rows = terminal_rows() * 2 / (2 / precision);

    if (strcmp(file, "-") == 0) {
        img_load_from_stdin(&img);
    } else {
        img_load_from_file(&img, file);
    }
    if (cols == 0 && rows == 0) {
        scale_cols = max_cols / (float)img.width;
        scale_rows = max_rows / (float)img.height;
        if (adjust_to_height && scale_rows < scale_cols && max_rows < img.height)
            // rows == 0 and adjust_to_height > adjust to height instead of width
            img_resize(&img, scale_rows, scale_rows);
        else if (max_cols < img.width)
            img_resize(&img, scale_cols, scale_cols);
    } else if (cols > 0 && cols < img.width) {
        scale_cols = cols / (float)img.width;
        img_resize(&img, scale_cols, scale_cols);
     } else if (rows > 0 && rows < img.height) {
        scale_rows = (float)(rows * 2) / (float)img.height;
        img_resize(&img, scale_rows, scale_rows);
    }

    if (convert)
        img_convert_colors(&img);
    /*printf("Loaded %s: %ux%u. Console width: %u\n", file, img.width, img.height, cols);*/
    // For GIF
    if (img.frames > 1) {
        system("clear");
        signal(SIGINT, intHandler);
    } else {
        loops = 0;
    }

    display_image(stdout, &img, true_color, precision);

    img_free(&img);
    free_hash_colors();
    return 0;
}
