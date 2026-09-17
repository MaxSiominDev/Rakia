#ifndef MATERIAL_H
#define MATERIAL_H

#include "engine/gl_compat.h"
#include "engine/obj.h"
#include "engine/shader.h"
#include "engine/texture.h"
#include "engine/vecmath.h"

// texture units the mesh and depth shaders sample
#define MATERIAL_UNIT_DIFFUSE 0
#define MATERIAL_UNIT_NORMAL 1
#define MATERIAL_UNIT_METAL_ROUGH 2
#define MATERIAL_UNIT_EMISSIVE 3

typedef struct {
    Vec3 kd;
    float opacity;
    Vec3 emissive;
    float metallic;
    float roughness;
    GLuint diffuse_map;
    GLuint normal_map;
    // glTF packing: green is roughness, blue metallic, each scaled by the factor above
    GLuint metal_rough_map;
    GLuint emissive_map;
    int alpha_test;
    int blend;
    int double_sided;
} Material;

// 1x1 stand-ins replace missing and unreadable maps, and every material binds all four units
void material_init(void);
void material_default(Material *material);
void material_load(Material *material, const ObjMaterial *source, const char *directory);
// each file is uploaded once; 0 when it cannot be read
GLuint material_texture(const char *relative, TextureKind kind);
void material_bind(const Material *material, Shader *shader);

#endif
