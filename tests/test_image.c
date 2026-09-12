#include "check.h"
#include "engine/image.h"

#include <stddef.h>

#define PANORAMA "assets/raw/sky/belfast_sunset_puresky/belfast_sunset_puresky_2k.hdr"
#define NORMAL_MAP "assets/raw/aircraft/f16_rickslash/f16_rickslash_normal_2.jpg"

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

void test_image_main(void)
{
    HdrImage panorama;
    Image normal_map;
    const char *reason = NULL;

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

    check(image_load(&normal_map, "assets/raw/nothing.png", &reason) == -1 && reason != NULL,
          "a missing image fails with a reason");
}
