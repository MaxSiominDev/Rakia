#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>

typedef struct {
    int windowed;
    int window_width;
    int window_height;
    const char *screenshot_path;
    int bench_frames;
    const char *assets_dir;
} Options;

int options_parse(Options *options, int argc, char **argv, char *error, size_t error_size);
void options_print_usage(const char *program);

#endif
