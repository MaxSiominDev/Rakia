#ifndef SHADER_H
#define SHADER_H

#include "engine/gl_compat.h"

#define SHADER_ATTRIBUTE_POSITION 0
#define SHADER_ATTRIBUTE_NORMAL 1
#define SHADER_ATTRIBUTE_UV 2
#define SHADER_ATTRIBUTE_TANGENT 3

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

#endif
