#include "engine/options.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_window_size(const char *text, int *width, int *height)
{
    char trailing;

    if (sscanf(text, "%dx%d%c", width, height, &trailing) != 2) {
        return -1;
    }

    return *width > 0 && *height > 0 ? 0 : -1;
}

static int parse_positive_int(const char *text, int *value)
{
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno != 0 || parsed < 1 || parsed > INT_MAX) {
        return -1;
    }

    *value = (int)parsed;
    return 0;
}

int options_parse(Options *options, int argc, char **argv, char *error, size_t error_size)
{
    int i;

    memset(options, 0, sizeof *options);

    for (i = 1; i < argc; i += 2) {
        const char *flag = argv[i];
        const char *value = i + 1 < argc ? argv[i + 1] : NULL;

        if (strcmp(flag, "--window") == 0) {
            if (value == NULL || parse_window_size(value, &options->window_width, &options->window_height) != 0) {
                snprintf(error, error_size, "--window needs a size like 1280x720");
                return -1;
            }
            options->windowed = 1;
        } else if (strcmp(flag, "--screenshot") == 0) {
            if (value == NULL) {
                snprintf(error, error_size, "--screenshot needs a file name");
                return -1;
            }
            options->screenshot_path = value;
        } else if (strcmp(flag, "--bench") == 0) {
            if (value == NULL || parse_positive_int(value, &options->bench_frames) != 0) {
                snprintf(error, error_size, "--bench needs a positive frame count");
                return -1;
            }
        } else if (strcmp(flag, "--assets") == 0) {
            if (value == NULL) {
                snprintf(error, error_size, "--assets needs a directory");
                return -1;
            }
            options->assets_dir = value;
        } else {
            snprintf(error, error_size, "unknown option: %s", flag);
            return -1;
        }
    }

    if (options->screenshot_path != NULL && options->bench_frames > 0) {
        snprintf(error, error_size, "--screenshot and --bench cannot be combined");
        return -1;
    }

    return 0;
}

void options_print_usage(const char *program)
{
    fprintf(stderr,
            "usage: %s [--window WxH] [--screenshot FILE] [--bench N] [--assets DIR]\n"
            "  --window WxH      run in a window of that size instead of fullscreen\n"
            "  --screenshot FILE render one frame into FILE as BMP, then exit\n"
            "  --bench N         render N frames, print the mean frame time, then exit\n"
            "  --assets DIR      load shaders and textures from DIR instead of the\n"
            "                    assets directory next to the executable\n"
            "\n"
            "keys: F11 fullscreen  Esc quit\n",
            program);
}
