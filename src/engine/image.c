#include "engine/image.h"

#include "engine/assets.h"

#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int image_load(Image *image, const char *relative, const char **reason)
{
    unsigned char *data;
    size_t size;

    if (assets_read(relative, &data, &size, reason) != 0) {
        return -1;
    }
    stbi_set_flip_vertically_on_load(1);
    image->pixels = stbi_load_from_memory(data, (int)size, &image->width, &image->height, &image->channels, 0);
    free(data);
    if (image->pixels == NULL) {
        *reason = stbi_failure_reason();
        return -1;
    }

    return 0;
}

// every frame is read as rgba, so the cells are copied without looking at what the file held
static int load_rgba(Image *image, const char *relative, const char **reason)
{
    unsigned char *data;
    size_t size;

    if (assets_read(relative, &data, &size, reason) != 0) {
        return -1;
    }
    stbi_set_flip_vertically_on_load(1);
    image->pixels = stbi_load_from_memory(data, (int)size, &image->width, &image->height, &image->channels, 4);
    free(data);
    if (image->pixels == NULL) {
        *reason = stbi_failure_reason();
        return -1;
    }
    image->channels = 4;

    return 0;
}

static void copy_frame(const Image *atlas, const Image *frame, int left, int bottom)
{
    int row;

    for (row = 0; row < frame->height; row++) {
        memcpy(atlas->pixels + (((size_t)(bottom + row) * (size_t)atlas->width) + (size_t)left) * 4,
               frame->pixels + (size_t)row * (size_t)frame->width * 4, (size_t)frame->width * 4);
    }
}

int image_atlas(Image *atlas, const char *const *paths, int count, int columns, const char **reason)
{
    const int rows = (count + columns - 1) / columns;
    Image *frames = calloc((size_t)count, sizeof *frames);
    int cell_width = 0;
    int cell_height = 0;
    int i;

    if (frames == NULL) {
        *reason = "out of memory";
        return -1;
    }
    for (i = 0; i < count; i++) {
        if (load_rgba(&frames[i], paths[i], reason) != 0) {
            while (i-- > 0) {
                image_free(&frames[i]);
            }
            free(frames);
            return -1;
        }
        cell_width = frames[i].width > cell_width ? frames[i].width : cell_width;
        cell_height = frames[i].height > cell_height ? frames[i].height : cell_height;
    }

    atlas->width = cell_width * columns;
    atlas->height = cell_height * rows;
    atlas->channels = 4;
    atlas->pixels = calloc((size_t)atlas->width * (size_t)atlas->height, 4);
    if (atlas->pixels == NULL) {
        *reason = "out of memory";
    }
    for (i = 0; i < count; i++) {
        if (atlas->pixels != NULL) {
            copy_frame(atlas, &frames[i],
                       (i % columns) * cell_width + (cell_width - frames[i].width) / 2,
                       (i / columns) * cell_height + (cell_height - frames[i].height) / 2);
        }
        image_free(&frames[i]);
    }
    free(frames);

    return atlas->pixels != NULL ? 0 : -1;
}

// reads only the previous pass, so each pass grows the colored area by one texel
static void bleed_pass(unsigned char *work, const unsigned char *before, int width, int height)
{
    int x;
    int y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            const unsigned char *center = before + ((size_t)y * (size_t)width + (size_t)x) * 4;
            unsigned char *out = work + ((size_t)y * (size_t)width + (size_t)x) * 4;
            int sum[3] = {0, 0, 0};
            int count = 0;
            int dx;
            int dy;

            if (center[3] != 0) {
                continue;
            }
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    const int nx = x + dx;
                    const int ny = y + dy;
                    const unsigned char *neighbor;

                    if ((dx == 0 && dy == 0) || nx < 0 || nx >= width || ny < 0 || ny >= height) {
                        continue;
                    }
                    neighbor = before + ((size_t)ny * (size_t)width + (size_t)nx) * 4;
                    if (neighbor[3] == 0) {
                        continue;
                    }
                    sum[0] += neighbor[0];
                    sum[1] += neighbor[1];
                    sum[2] += neighbor[2];
                    count++;
                }
            }
            if (count > 0) {
                out[0] = (unsigned char)(sum[0] / count);
                out[1] = (unsigned char)(sum[1] / count);
                out[2] = (unsigned char)(sum[2] / count);
                out[3] = 255;
            }
        }
    }
}

void image_bleed(Image *image, int passes)
{
    const size_t count = (size_t)image->width * (size_t)image->height;
    const size_t bytes = count * 4;
    unsigned char *work = malloc(bytes);
    unsigned char *before = malloc(bytes);
    size_t i;
    int pass;

    // a failed bleed only leaves the dark fringe, so the load goes on
    if (work == NULL || before == NULL) {
        free(work);
        free(before);
        return;
    }
    memcpy(work, image->pixels, bytes);
    for (pass = 0; pass < passes; pass++) {
        memcpy(before, work, bytes);
        bleed_pass(work, before, image->width, image->height);
    }
    for (i = 0; i < count; i++) {
        if (image->pixels[i * 4 + 3] == 0) {
            image->pixels[i * 4] = work[i * 4];
            image->pixels[i * 4 + 1] = work[i * 4 + 1];
            image->pixels[i * 4 + 2] = work[i * 4 + 2];
        }
    }
    free(work);
    free(before);
}

void image_free(Image *image)
{
    stbi_image_free(image->pixels);
    image->pixels = NULL;
}

int image_load_hdr(HdrImage *image, const char *relative, const char **reason)
{
    unsigned char *data;
    size_t size;
    int channels;

    if (assets_read(relative, &data, &size, reason) != 0) {
        return -1;
    }
    stbi_set_flip_vertically_on_load(1);
    image->pixels = stbi_loadf_from_memory(data, (int)size, &image->width, &image->height, &channels, 3);
    free(data);
    if (image->pixels == NULL) {
        *reason = stbi_failure_reason();
        return -1;
    }

    return 0;
}

void image_free_hdr(HdrImage *image)
{
    stbi_image_free(image->pixels);
    image->pixels = NULL;
}
