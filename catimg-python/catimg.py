#!/usr/bin/env python3

import sys
import tempfile
from cffi import FFI

class CatImg(object):
    def __init__(self, libpath='/usr/lib/libcatimg.so'):
        includes = """
            // Required types
            typedef struct {
                    uint8_t r, ///< Red
                            g, ///< Green
                            b, ///< Blue
                            a; ///< Alpha
            } color_t;
            typedef struct {
                    color_t *pixels; ///< 1 dim pixels pixels[x+y*w]
                    uint32_t width, ///< Width of the image
                             height, ///< Height of the image
                             frames; ///< Number of frames
                    uint16_t *delays; ///< Array of delays. Length = frames - 1
            } image_t;

            // Functions
            void display_image(FILE *, image_t *, int, int);
            void img_load_from_file(image_t *, const char *);
            void img_resize(image_t *, float, float);
            void img_free(image_t *);
            extern int loops;
        """
        self.ffifactory = FFI()

        self.ffifactory.cdef(includes)

        self.lib = self.ffifactory.dlopen(libpath)

    def get_image(self, path):
        cpath = self.ffifactory.new("char[]", path.encode('utf8'))
        img = self.ffifactory.new("image_t *")
        self.lib.loops = 0

        self.lib.img_load_from_file(img, cpath)

        self.lib.img_resize(img, 0.1, 0.1)
        f = tempfile.TemporaryFile()
        self.lib.display_image(f, img, 0, 2)

        f.seek(0)
        databuf = f.read()

        f.close()
        self.lib.img_free(img)


        return databuf.decode('utf8')

if __name__=="__main__":
    ci = CatImg(libpath=sys.argv[1])
    sys.stdout.write(ci.get_image(sys.argv[2]))
