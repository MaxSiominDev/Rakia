#include "engine/shader.h"

#include "engine/assets.h"
#include "engine/gl_ext.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_CAPACITY 4096

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *text;

    if (file == NULL) {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "cannot read %s: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }

    text = malloc((size_t)size + 1);
    if (text == NULL || fread(text, 1, (size_t)size, file) != (size_t)size) {
        fprintf(stderr, "cannot read %s\n", path);
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);

    return text;
}

static int source_path(char *out, const char *name, const char *extension)
{
    char relative[256];

    snprintf(relative, sizeof relative, "shaders/%s.%s", name, extension);
    if (assets_path(out, ASSETS_PATH_MAX, relative) != 0) {
        fprintf(stderr, "shader path is too long: %s\n", relative);
        return -1;
    }

    return 0;
}

static GLuint compile(GLenum type, const char *path)
{
    char *source = read_file(path);
    const GLchar *sources[1];
    GLuint shader;
    GLint compiled = GL_FALSE;

    if (source == NULL) {
        return 0;
    }

    shader = glCreateShader(type);
    sources[0] = source;
    glShaderSource(shader, 1, sources, NULL);
    glCompileShader(shader);
    free(source);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[LOG_CAPACITY];

        glGetShaderInfoLog(shader, sizeof log, NULL, log);
        fprintf(stderr, "%s: compile failed\n%s\n", path, log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint link(GLuint vertex, GLuint fragment, const char *name)
{
    GLuint program = glCreateProgram();
    GLint linked = GL_FALSE;

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glBindAttribLocation(program, SHADER_ATTRIBUTE_POSITION, "a_position");
    glBindAttribLocation(program, SHADER_ATTRIBUTE_NORMAL, "a_normal");
    glBindAttribLocation(program, SHADER_ATTRIBUTE_UV, "a_uv");
    glBindAttribLocation(program, SHADER_ATTRIBUTE_TANGENT, "a_tangent");
    glBindAttribLocation(program, SHADER_ATTRIBUTE_CORNER, "a_corner");
    glBindAttribLocation(program, SHADER_ATTRIBUTE_COLOR, "a_color");
    glLinkProgram(program);
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[LOG_CAPACITY];

        glGetProgramInfoLog(program, sizeof log, NULL, log);
        fprintf(stderr, "shaders/%s.vert + shaders/%s.frag: link failed\n%s\n", name, name, log);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

int shader_load(Shader *shader, const char *name)
{
    char vertex_path[ASSETS_PATH_MAX];
    char fragment_path[ASSETS_PATH_MAX];
    GLuint vertex;
    GLuint fragment;
    GLuint program;

    memset(shader, 0, sizeof *shader);
    if (source_path(vertex_path, name, "vert") != 0 || source_path(fragment_path, name, "frag") != 0) {
        return -1;
    }

    vertex = compile(GL_VERTEX_SHADER, vertex_path);
    if (vertex == 0) {
        return -1;
    }
    fragment = compile(GL_FRAGMENT_SHADER, fragment_path);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return -1;
    }

    program = link(vertex, fragment, name);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (program == 0) {
        return -1;
    }

    shader->program = program;
    return 0;
}

GLint shader_uniform(Shader *shader, const char *name)
{
    GLint location;
    int i;

    for (i = 0; i < shader->uniform_count; i++) {
        if (strcmp(shader->uniforms[i].name, name) == 0) {
            return shader->uniforms[i].location;
        }
    }

    location = glGetUniformLocation(shader->program, name);
    if (shader->uniform_count < SHADER_UNIFORM_CACHE && strlen(name) < SHADER_UNIFORM_NAME_LENGTH) {
        strcpy(shader->uniforms[shader->uniform_count].name, name);
        shader->uniforms[shader->uniform_count].location = location;
        shader->uniform_count++;
    }

    return location;
}

void shader_set_int(Shader *shader, const char *name, int value)
{
    glUniform1i(shader_uniform(shader, name), value);
}

void shader_set_float(Shader *shader, const char *name, float value)
{
    glUniform1f(shader_uniform(shader, name), value);
}

void shader_set_vec3(Shader *shader, const char *name, Vec3 value)
{
    glUniform3f(shader_uniform(shader, name), value.x, value.y, value.z);
}

void shader_set_mat4(Shader *shader, const char *name, Mat4 value)
{
    glUniformMatrix4fv(shader_uniform(shader, name), 1, GL_FALSE, value.m);
}
