#include "check.h"
#include "engine/assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_explicit_directory(void)
{
    char path[64];

    assets_init("/data/rakia");
    check(assets_path(path, sizeof path, "shaders/mesh.vert") == 0, "path fits");
    check(strcmp(path, "/data/rakia/shaders/mesh.vert") == 0, "path joins the directory and the file");
}

static void test_truncation(void)
{
    char path[16];

    assets_init("/data/rakia");
    check(assets_path(path, sizeof path, "shaders/mesh.vert") == -1, "too long a path is refused");
}

static void test_read(void)
{
    unsigned char *data = NULL;
    size_t size = 0;
    const char *reason = NULL;
    char direct[4096];
    size_t direct_size = 0;
    FILE *file;

    assets_init("tests/fixtures");
    check(assets_read("shapes.obj", &data, &size, &reason) == 0, "a fixture file reads through the assets module");
    if (data != NULL) {
        file = fopen("tests/fixtures/shapes.obj", "rb");
        if (file != NULL) {
            direct_size = fread(direct, 1, sizeof direct, file);
            fclose(file);
        }
        check(size == direct_size, "the read size matches a direct fopen of the same file");
        check(memcmp(data, direct, size) == 0, "the read bytes match a direct fopen of the same file");
        check(data[size] == '\0', "the buffer is NUL-terminated one byte past size");
        free(data);
    }

    check(assets_read("does_not_exist_at_all.obj", &data, &size, &reason) == -1 && reason != NULL,
          "a missing file fails with a reason");
}

// shader.c has no GL context to compile against here, but its own read of "shaders/name.vert" through
// this same function is what this confirms
static void test_read_real_shader(void)
{
    unsigned char *data = NULL;
    size_t size = 0;
    const char *reason = NULL;
    char direct[8192];
    size_t direct_size = 0;
    FILE *file;

    assets_init("assets");
    check(assets_read("shaders/mesh.vert", &data, &size, &reason) == 0, "a real shader reads through the assets module");
    if (data != NULL) {
        file = fopen("assets/shaders/mesh.vert", "rb");
        if (file != NULL) {
            direct_size = fread(direct, 1, sizeof direct, file);
            fclose(file);
        }
        check(size == direct_size, "the shader's read size matches a direct fopen of the same file");
        check(memcmp(data, direct, size) == 0, "the shader's read bytes match a direct fopen of the same file");
        free(data);
    }
}

void test_assets_main(void)
{
    test_explicit_directory();
    test_truncation();
    test_read();
    test_read_real_shader();
}
