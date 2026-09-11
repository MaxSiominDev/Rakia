#include "engine/assets.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#else
#include <windows.h>
#endif

static char directory[ASSETS_PATH_MAX] = "assets";

static int executable_directory(char *out, size_t size)
{
    char *last_separator;
#ifdef __APPLE__
    char raw[ASSETS_PATH_MAX];
    char resolved[PATH_MAX];
    unsigned int raw_size = sizeof raw;

    if (_NSGetExecutablePath(raw, &raw_size) != 0 || realpath(raw, resolved) == NULL ||
        snprintf(out, size, "%s", resolved) >= (int)size) {
        return -1;
    }
#else
    const DWORD length = GetModuleFileNameA(NULL, out, (DWORD)size);

    if (length == 0 || length >= size) {
        return -1;
    }
#endif

    last_separator = strrchr(out, '/');
#ifdef _WIN32
    {
        char *backslash = strrchr(out, '\\');

        if (backslash != NULL && (last_separator == NULL || backslash > last_separator)) {
            last_separator = backslash;
        }
    }
#endif
    if (last_separator == NULL) {
        return -1;
    }
    *last_separator = '\0';

    return 0;
}

static int has_shaders(const char *candidate)
{
    char path[ASSETS_PATH_MAX];
    struct stat info;

    if (snprintf(path, sizeof path, "%s/shaders", candidate) >= (int)sizeof path) {
        return 0;
    }

    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

void assets_init(const char *dir)
{
    char exe_dir[ASSETS_PATH_MAX];

    if (dir != NULL) {
        snprintf(directory, sizeof directory, "%s", dir);
        return;
    }

    if (executable_directory(exe_dir, sizeof exe_dir) != 0) {
        fprintf(stderr, "cannot locate the executable, loading assets from the current directory\n");
        return;
    }

    if (snprintf(directory, sizeof directory, "%s/assets", exe_dir) >= (int)sizeof directory) {
        fprintf(stderr, "the executable path is too long, loading assets from the current directory\n");
        strcpy(directory, "assets");
        return;
    }
    if (has_shaders(directory)) {
        return;
    }

    fprintf(stderr, "no assets next to the executable (%s), loading them from the current directory\n", directory);
    strcpy(directory, "assets");
}

int assets_path(char *out, size_t size, const char *relative)
{
    const int length = snprintf(out, size, "%s/%s", directory, relative);

    return length >= 0 && (size_t)length < size ? 0 : -1;
}
