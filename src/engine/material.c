#include "engine/material.h"

#include "engine/gl_ext.h"

#include <stdio.h>
#include <string.h>

#define TEXTURE_CACHE_SIZE 128

static struct {
    char path[OBJ_PATH_MAX];
    TextureKind kind;
    GLuint id;
} cache[TEXTURE_CACHE_SIZE];
static int cache_count;

static GLuint white_map;
static GLuint flat_normal_map;

static GLuint solid_texture(unsigned char r, unsigned char g, unsigned char b)
{
    const unsigned char pixel[4] = {r, g, b, 255};
    GLuint id = 0;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glBindTexture(GL_TEXTURE_2D, 0);

    return id;
}

void material_init(void)
{
    white_map = solid_texture(255, 255, 255);
    flat_normal_map = solid_texture(128, 128, 255);
}

void material_default(Material *material)
{
    memset(material, 0, sizeof *material);
    material->kd = v3(1.0f, 1.0f, 1.0f);
    material->opacity = 1.0f;
    material->roughness = 1.0f;
    material->diffuse_map = white_map;
    material->normal_map = flat_normal_map;
    material->metal_rough_map = white_map;
    material->emissive_map = white_map;
}

GLuint material_texture(const char *relative, TextureKind kind)
{
    GLuint id;
    int i;

    for (i = 0; i < cache_count; i++) {
        if (cache[i].kind == kind && strcmp(cache[i].path, relative) == 0) {
            return cache[i].id;
        }
    }

    id = texture_load(relative, kind);
    if (id != 0 && cache_count < TEXTURE_CACHE_SIZE) {
        snprintf(cache[cache_count].path, sizeof cache[cache_count].path, "%s", relative);
        cache[cache_count].kind = kind;
        cache[cache_count].id = id;
        cache_count++;
    }

    return id;
}

// avoids a redundant "./" when directory has no directory component of its own
static GLuint load_map(const char *directory, const char *name, TextureKind kind, GLuint fallback)
{
    char path[OBJ_PATH_MAX * 2];
    GLuint id;

    if (name[0] == '\0') {
        return fallback;
    }

    if (strcmp(directory, ".") == 0) {
        snprintf(path, sizeof path, "%s", name);
    } else {
        snprintf(path, sizeof path, "%s/%s", directory, name);
    }
    id = material_texture(path, kind);

    return id != 0 ? id : fallback;
}

void material_load(Material *material, const ObjMaterial *source, const char *directory)
{
    material_default(material);
    material->kd = source->kd;
    material->opacity = source->opacity;
    material->emissive = source->ke;
    material->metallic = source->metallic;
    material->roughness = source->roughness;
    material->diffuse_map = load_map(directory, source->diffuse_map, TEXTURE_COLOR, white_map);
    material->normal_map = load_map(directory, source->normal_map, TEXTURE_DATA, flat_normal_map);
    material->metal_rough_map = load_map(directory, source->metal_rough_map, TEXTURE_DATA, white_map);
    material->emissive_map = load_map(directory, source->emissive_map, TEXTURE_COLOR, white_map);
    // the collected models keep their alpha in the diffuse image and name it again as map_d
    material->alpha_test = source->alpha_map[0] != '\0';
    material->blend = source->blend;
    material->double_sided = source->double_sided;
}

static void bind_unit(int unit, GLuint texture)
{
    glActiveTexture(GL_TEXTURE0 + (GLenum)unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void material_bind(const Material *material, Shader *shader)
{
    shader_set_vec3(shader, "u_kd", material->kd);
    shader_set_float(shader, "u_opacity", material->opacity);
    shader_set_vec3(shader, "u_emissive", material->emissive);
    shader_set_float(shader, "u_metallic", material->metallic);
    shader_set_float(shader, "u_roughness", material->roughness);
    bind_unit(MATERIAL_UNIT_DIFFUSE, material->diffuse_map);
    bind_unit(MATERIAL_UNIT_NORMAL, material->normal_map);
    bind_unit(MATERIAL_UNIT_METAL_ROUGH, material->metal_rough_map);
    bind_unit(MATERIAL_UNIT_EMISSIVE, material->emissive_map);
    glActiveTexture(GL_TEXTURE0);
}
