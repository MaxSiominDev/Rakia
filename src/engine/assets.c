#include "engine/assets.h"

#include "engine/pack.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#else
#include <windows.h>
#endif

static char directory[ASSETS_PATH_MAX] = "assets";
static Pack pack;
static int pack_loaded;
// set once assets_init was given an explicit directory, so a pack never shadows a developer's --assets override
static int forced_disk;

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

static const unsigned char *embedded_pack(size_t *size)
{
#ifdef __APPLE__
    // __RAKIA/__pack must match the segment and section names the release build embeds the pack under
    const struct mach_header_64 *header = (const struct mach_header_64 *)_dyld_get_image_header(0);
    unsigned long section_size = 0;
    unsigned char *section = getsectiondata(header, "__RAKIA", "__pack", &section_size);

    *size = section_size;

    return section;
#else
    // PACK/RT_RCDATA must match the resource name the release build embeds the pack under
    HMODULE module = GetModuleHandleA(NULL);
    HRSRC resource = FindResourceA(module, "PACK", RT_RCDATA);
    HGLOBAL handle;

    *size = 0;
    if (resource == NULL) {
        return NULL;
    }
    handle = LoadResource(module, resource);
    if (handle == NULL) {
        return NULL;
    }
    *size = SizeofResource(module, resource);

    return (const unsigned char *)LockResource(handle);
#endif
}

void assets_init(const char *dir)
{
    char exe_dir[ASSETS_PATH_MAX];
    const unsigned char *pack_data;
    size_t pack_size;

    if (dir != NULL) {
        forced_disk = 1;
        snprintf(directory, sizeof directory, "%s", dir);
        return;
    }

    pack_data = embedded_pack(&pack_size);
    if (pack_data != NULL && pack_open(&pack, pack_data, pack_size) == 0) {
        pack_loaded = 1;
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

int assets_read(const char *relative, unsigned char **data, size_t *size, const char **reason)
{
    char disk_path[ASSETS_PATH_MAX];
    const unsigned char *found;
    size_t found_size;
    unsigned char *buffer;
    FILE *file;
    long length;

    if (!forced_disk && pack_loaded) {
        if (pack_find(&pack, relative, &found, &found_size) != 0) {
            *reason = "not found in the asset pack";
            return -1;
        }
        buffer = malloc(found_size + 1);
        if (buffer == NULL) {
            *reason = "out of memory";
            return -1;
        }
        memcpy(buffer, found, found_size);
        buffer[found_size] = '\0';
        *data = buffer;
        *size = found_size;

        return 0;
    }

    if (assets_path(disk_path, sizeof disk_path, relative) != 0) {
        *reason = "the asset path is too long";
        return -1;
    }
    file = fopen(disk_path, "rb");
    if (file == NULL) {
        *reason = strerror(errno);
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0) {
        *reason = strerror(errno);
        fclose(file);
        return -1;
    }
    buffer = malloc((size_t)length + 1);
    if (buffer == NULL) {
        *reason = "out of memory";
        fclose(file);
        return -1;
    }
    if (fread(buffer, 1, (size_t)length, file) != (size_t)length) {
        *reason = "read error";
        free(buffer);
        fclose(file);
        return -1;
    }
    fclose(file);
    buffer[length] = '\0';

    *data = buffer;
    *size = (size_t)length;

    return 0;
}
