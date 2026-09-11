#include "check.h"
#include "engine/assets.h"

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

void test_assets_main(void)
{
    test_explicit_directory();
    test_truncation();
}
