#include "engine/image.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static FILE *open_image(const char *path, const char **reason)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        *reason = strerror(errno);
        return NULL;
    }
    stbi_set_flip_vertically_on_load(1);

    return file;
}

int image_load(Image *image, const char *path, const char **reason)
{
    FILE *file = open_image(path, reason);

    if (file == NULL) {
        return -1;
    }

    image->pixels = stbi_load_from_file(file, &image->width, &image->height, &image->channels, 0);
    fclose(file);
    if (image->pixels == NULL) {
        *reason = stbi_failure_reason();
        return -1;
    }

    return 0;
}

// every frame is read as rgba, so the cells are copied without looking at what the file held
static int load_rgba(Image *image, const char *path, const char **reason)
{
    FILE *file = open_image(path, reason);

    if (file == NULL) {
        return -1;
    }

    image->pixels = stbi_load_from_file(file, &image->width, &image->height, &image->channels, 4);
    fclose(file);
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

void image_free(Image *image)
{
    stbi_image_free(image->pixels);
    image->pixels = NULL;
}

int image_load_hdr(HdrImage *image, const char *path, const char **reason)
{
    FILE *file = open_image(path, reason);
    int channels;

    if (file == NULL) {
        return -1;
    }

    image->pixels = stbi_loadf_from_file(file, &image->width, &image->height, &channels, 3);
    fclose(file);
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
