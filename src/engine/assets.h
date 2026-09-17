#ifndef ASSETS_H
#define ASSETS_H

#include <stddef.h>

#define ASSETS_PATH_MAX 1024

// with dir NULL the assets are looked for next to the executable, then in the working directory
void assets_init(const char *dir);
int assets_path(char *out, size_t size, const char *relative);
// reads a whole file relative to the assets root: from the embedded pack if the executable carries one
// and no --assets dir was forced, otherwise from disk like assets_path resolves it. *data comes back
// malloc'd and NUL-terminated one byte past *size, for callers that want a C string; free it with free().
// -1 on failure, with *reason set to a short phrase for "cannot open %s: %s"
int assets_read(const char *relative, unsigned char **data, size_t *size, const char **reason);

#endif
