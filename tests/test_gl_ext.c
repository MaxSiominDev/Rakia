#include "check.h"
#include "engine/gl_ext.h"

#include <stddef.h>

void test_gl_ext_main(void)
{
    const char *list = "GL_EXT_texture3D GL_EXT_texture_sRGB_decode GL_EXT_texture_sRGB GL_ARB_shadow";

    check(gl_ext_list_has(list, "GL_EXT_texture3D") == 1, "first entry is found");
    check(gl_ext_list_has(list, "GL_ARB_shadow") == 1, "last entry is found");
    check(gl_ext_list_has(list, "GL_EXT_texture_sRGB") == 1, "entry after a longer one with the same prefix is found");
    check(gl_ext_list_has(list, "GL_EXT_texture") == 0, "a prefix of an entry does not match");
    check(gl_ext_list_has(list, "texture_sRGB") == 0, "a suffix of an entry does not match");
    check(gl_ext_list_has(list, "GL_EXT_texture_sRGB_decode_x") == 0, "a longer name does not match");
    check(gl_ext_list_has("", "GL_ARB_shadow") == 0, "empty list has nothing");
    check(gl_ext_list_has(NULL, "GL_ARB_shadow") == 0, "missing list has nothing");
    check(gl_ext_list_has("GL_ARB_shadow", "GL_ARB_shadow") == 1, "single entry list");
}
