#ifndef ASSETS_H
#define ASSETS_H

#include <stddef.h>

#define ASSETS_PATH_MAX 1024

// with dir NULL the assets are looked for next to the executable, then in the working directory
void assets_init(const char *dir);
int assets_path(char *out, size_t size, const char *relative);

#endif
