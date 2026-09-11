#include "engine/screenshot.h"

#include "engine/gl_compat.h"

#include <stdio.h>
#include <stdlib.h>

static void put_u16(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)(value & 0xff);
    p[1] = (unsigned char)((value >> 8) & 0xff);
}

static void put_u32(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)(value & 0xff);
    p[1] = (unsigned char)((value >> 8) & 0xff);
    p[2] = (unsigned char)((value >> 16) & 0xff);
    p[3] = (unsigned char)((value >> 24) & 0xff);
}

int screenshot_save_bmp(const char *path, int width, int height)
{
    const size_t stride = (((size_t)width * 3) + 3) & ~(size_t)3;
    const size_t data_size = stride * (size_t)height;
    unsigned char header[54] = {0};
    unsigned char *pixels;
    FILE *file;
    int ok;

    if (width <= 0 || height <= 0) {
        return -1;
    }

    pixels = malloc(data_size);
    if (pixels == NULL) {
        return -1;
    }

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels);

    header[0] = 'B';
    header[1] = 'M';
    put_u32(header + 2, (unsigned)(sizeof header + data_size));
    put_u32(header + 10, (unsigned)sizeof header);
    put_u32(header + 14, 40);
    put_u32(header + 18, (unsigned)width);
    put_u32(header + 22, (unsigned)height);
    put_u16(header + 26, 1);
    put_u16(header + 28, 24);
    put_u32(header + 34, (unsigned)data_size);

    file = fopen(path, "wb");
    if (file == NULL) {
        free(pixels);
        return -1;
    }

    ok = fwrite(header, sizeof header, 1, file) == 1 &&
         fwrite(pixels, data_size, 1, file) == 1;

    fclose(file);
    free(pixels);

    return ok ? 0 : -1;
}
