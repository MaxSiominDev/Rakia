#ifndef PLATFORM_H
#define PLATFORM_H

#include "engine/input.h"
#include "engine/options.h"

typedef struct {
    void *context;
    // runs once the GL context exists; a non-zero result ends the program
    int (*init)(void *context);
    // runs TIMESTEP_HZ times per simulated second, dt in seconds
    void (*update)(void *context, const Input *input, float dt);
    void (*render)(void *context, int width, int height);
} PlatformApp;

int platform_run(const Options *options, const PlatformApp *app, int *argc, char **argv);

#endif
