#include "engine/gl_ext.h"

#include <stdio.h>
#include <string.h>

static const struct {
    const char *name;
    int required;
} extensions[GLEXT_COUNT] = {
    {"GL_ARB_shader_objects", 1},
    {"GL_ARB_vertex_shader", 1},
    {"GL_ARB_fragment_shader", 1},
    {"GL_ARB_vertex_buffer_object", 1},
    {"GL_EXT_framebuffer_object", 0},
    {"GL_EXT_framebuffer_multisample", 0},
    {"GL_EXT_framebuffer_blit", 0},
    {"GL_ARB_texture_float", 0},
    {"GL_EXT_texture_sRGB", 0},
    {"GL_ARB_texture_non_power_of_two", 0},
    {"GL_EXT_texture_filter_anisotropic", 0}
};

static int present[GLEXT_COUNT];

int gl_ext_list_has(const char *list, const char *name)
{
    const size_t length = strlen(name);
    const char *hit = list;

    while (hit != NULL && (hit = strstr(hit, name)) != NULL) {
        const char after = hit[length];

        if ((hit == list || hit[-1] == ' ') && (after == ' ' || after == '\0')) {
            return 1;
        }
        hit += length;
    }

    return 0;
}

#ifdef __APPLE__

#include <OpenGL/OpenGL.h>

static int resolve_entry_points(const char **missing)
{
    (void)missing;
    return 0;
}

void gl_ext_set_swap_interval(int interval)
{
    const GLint value = interval;

    CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &value);
}

#else

#define GL_EXT_DEFINE(gate, type, name) type name;
GL_ENTRY_POINTS(GL_EXT_DEFINE)
#undef GL_EXT_DEFINE

typedef void (*GenericProc)(void);

// wglGetProcAddress can return 0..3 or -1 on failure, not just NULL
static GenericProc resolve(const char *name)
{
    PROC address = wglGetProcAddress(name);

    if (address == (PROC)0 || address == (PROC)1 || address == (PROC)2 ||
        address == (PROC)3 || address == (PROC)-1) {
        return NULL;
    }

    return (GenericProc)address;
}

static int wanted(int gate)
{
    return gate == GLEXT_NONE || present[gate];
}

#define GL_EXT_RESOLVE(gate, type, name)             \
    if (wanted(gate)) {                              \
        name = (type)resolve(#name);                 \
        if (name == NULL) {                          \
            *missing = #name;                        \
            return -1;                               \
        }                                            \
    }

static int resolve_entry_points(const char **missing)
{
    GL_ENTRY_POINTS(GL_EXT_RESOLVE)
    return 0;
}

#undef GL_EXT_RESOLVE

void gl_ext_set_swap_interval(int interval)
{
    typedef BOOL (WINAPI *SwapIntervalProc)(int);
    const SwapIntervalProc swap_interval = (SwapIntervalProc)resolve("wglSwapIntervalEXT");

    if (swap_interval != NULL) {
        swap_interval(interval);
    }
}

#endif

int gl_ext_load(const char **missing)
{
    const char *list = (const char *)glGetString(GL_EXTENSIONS);
    int i;

    for (i = 0; i < GLEXT_COUNT; i++) {
        present[i] = gl_ext_list_has(list, extensions[i].name);
    }
    for (i = 0; i < GLEXT_COUNT; i++) {
        if (extensions[i].required && !present[i]) {
            *missing = extensions[i].name;
            return -1;
        }
    }

    return resolve_entry_points(missing);
}

void gl_ext_print_status(void)
{
    int i;

    for (i = 0; i < GLEXT_COUNT; i++) {
        printf("%s: %s%s\n", extensions[i].name, present[i] ? "present" : "missing",
               extensions[i].required ? " (required)" : "");
    }
}

int gl_ext_present(GlExtension extension)
{
    return present[extension];
}
