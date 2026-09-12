#ifndef OBJ_H
#define OBJ_H

#include "engine/mesh_data.h"

#define OBJ_NAME_MAX 64
#define OBJ_PATH_MAX 1024

// map paths are relative to Model.directory; absent factors read as Kd 1, d 1, Ke 0, Pm 0, Pr 1
typedef struct {
    char name[OBJ_NAME_MAX];
    Vec3 kd;
    float opacity;
    Vec3 ke;
    float metallic;
    float roughness;
    char diffuse_map[OBJ_PATH_MAX];
    char alpha_map[OBJ_PATH_MAX];
    char normal_map[OBJ_PATH_MAX];
    char metal_rough_map[OBJ_PATH_MAX];
    char emissive_map[OBJ_PATH_MAX];
    int blend;
    int double_sided;
} ObjMaterial;

typedef struct {
    MeshData data;
    ObjMaterial *materials;
    int material_count;
    char directory[OBJ_PATH_MAX];
} Model;

// groups with no usemtl get material -1; faces before the first g line form a group named ""
int obj_load(Model *model, const char *path);
void obj_free(Model *model);

#endif
