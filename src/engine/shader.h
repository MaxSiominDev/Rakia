#ifndef SHADER_H
#define SHADER_H

#include "engine/gl_compat.h"
#include "engine/vecmath.h"

#define SHADER_ATTRIBUTE_POSITION 0
#define SHADER_ATTRIBUTE_NORMAL 1
#define SHADER_ATTRIBUTE_UV 2
#define SHADER_ATTRIBUTE_TANGENT 3
// billboards use this in place of the normal and tangent slots, and particles a color of their own
#define SHADER_ATTRIBUTE_CORNER 4
#define SHADER_ATTRIBUTE_COLOR 5

#define SHADER_UNIFORM_CACHE 32
#define SHADER_UNIFORM_NAME_LENGTH 32

typedef struct {
    GLuint program;
    struct {
        char name[SHADER_UNIFORM_NAME_LENGTH];
        GLint location;
    } uniforms[SHADER_UNIFORM_CACHE];
    int uniform_count;
} Shader;

// loads shaders/NAME.vert and shaders/NAME.frag from the assets directory
int shader_load(Shader *shader, const char *name);
GLint shader_uniform(Shader *shader, const char *name);
void shader_set_int(Shader *shader, const char *name, int value);
void shader_set_float(Shader *shader, const char *name, float value);
void shader_set_vec3(Shader *shader, const char *name, Vec3 value);
void shader_set_mat4(Shader *shader, const char *name, Mat4 value);

#endif
