#include "check.h"
#include "engine/assets.h"
#include "engine/image.h"

#include <stddef.h>
#include <stdlib.h>

#define PANORAMA "raw/sky/belfast_sunset_puresky/belfast_sunset_puresky_2k.hdr"
#define NORMAL_MAP "raw/aircraft/f16_rickslash/f16_rickslash_normal_2.jpg"

static float row_luminance(const HdrImage *image, int row)
{
    const float *pixel = image->pixels + 3 * row * image->width;
    float sum = 0.0f;
    int x;

    for (x = 0; x < image->width; x++, pixel += 3) {
        sum += 0.2126f * pixel[0] + 0.7152f * pixel[1] + 0.0722f * pixel[2];
    }

    return sum / (float)image->width;
}

static Image make_image(int width, int height)
{
    Image image;

    image.width = width;
    image.height = height;
    image.channels = 4;
    image.pixels = calloc((size_t)width * (size_t)height, 4);

    return image;
}

static void set_pixel(Image *image, int x, int y, unsigned char r, unsigned char g, unsigned char b,
                      unsigned char a)
{
    unsigned char *pixel = image->pixels + ((size_t)y * (size_t)image->width + (size_t)x) * 4;

    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = a;
}

static const unsigned char *pixel_at(const Image *image, int x, int y)
{
    return image->pixels + ((size_t)y * (size_t)image->width + (size_t)x) * 4;
}

static void test_bleed(void)
{
    Image image = make_image(3, 3);
    const unsigned char *pixel;

    set_pixel(&image, 1, 1, 200, 40, 10, 255);
    image_bleed(&image, 1);
    pixel = pixel_at(&image, 0, 0);
    check(pixel[0] == 200 && pixel[1] == 40 && pixel[2] == 10, "a texel diagonal to an opaque one takes its color");
    check(pixel[3] == 0, "bleeding leaves alpha alone");
    free(image.pixels);

    image = make_image(5, 1);
    set_pixel(&image, 0, 0, 90, 60, 30, 255);
    image_bleed(&image, 1);
    pixel = pixel_at(&image, 2, 0);
    check(pixel[0] == 0 && pixel[3] == 0, "one pass does not reach two texels away");
    free(image.pixels);

    image = make_image(5, 1);
    set_pixel(&image, 0, 0, 90, 60, 30, 255);
    image_bleed(&image, 2);
    pixel = pixel_at(&image, 2, 0);
    check(pixel[0] == 90 && pixel[1] == 60 && pixel[2] == 30 && pixel[3] == 0,
          "a second pass reaches the texel the first pass could not, alpha still untouched");
    free(image.pixels);

    image = make_image(3, 3);
    image_bleed(&image, 4);
    pixel = pixel_at(&image, 1, 1);
    check(pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 0,
          "an image with nothing opaque anywhere is left untouched");
    free(image.pixels);
}

void test_image_main(void)
{
    HdrImage panorama;
    Image normal_map;
    const char *reason = NULL;

    assets_init("assets");
    test_bleed();

    check(image_load_hdr(&panorama, PANORAMA, &reason) == 0, "the sunset panorama loads");
    if (panorama.pixels != NULL) {
        check(panorama.width == 2048 && panorama.height == 1024, "the panorama is 2048x1024");
        check(row_luminance(&panorama, panorama.height - 1) > row_luminance(&panorama, 0),
              "rows are bottom-up, so the last row is the brighter zenith");
        image_free_hdr(&panorama);
        check(panorama.pixels == NULL, "free clears the panorama");
    }

    check(image_load(&normal_map, NORMAL_MAP, &reason) == 0, "a jpg loads");
    if (normal_map.pixels != NULL) {
        check(normal_map.width == 2048 && normal_map.height == 2048 && normal_map.channels == 3,
              "the normal map is 2048x2048 rgb");
        image_free(&normal_map);
    }

    check(image_load(&normal_map, "raw/nothing.png", &reason) == -1 && reason != NULL,
          "a missing image fails with a reason");
}
