#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    int windowed;
    int window_width;
    int window_height;
    int no_hidpi;
    const char *screenshot_path;
    int bench_frames;
    const char *assets_dir;
    int help;
} Options;

int options_parse(Options *options, int argc, char **argv, char *error, size_t error_size);
void options_print_usage(FILE *out, const char *program);

#endif
