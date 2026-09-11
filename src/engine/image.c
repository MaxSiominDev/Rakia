#include "engine/image.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int image_load(Image *image, const char *path, const char **reason)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        *reason = strerror(errno);
        return -1;
    }

    stbi_set_flip_vertically_on_load(1);
    image->pixels = stbi_load_from_file(file, &image->width, &image->height, &image->channels, 0);
    fclose(file);
    if (image->pixels == NULL) {
        *reason = stbi_failure_reason();
        return -1;
    }

    return 0;
}

void image_free(Image *image)
{
    stbi_image_free(image->pixels);
    image->pixels = NULL;
}
