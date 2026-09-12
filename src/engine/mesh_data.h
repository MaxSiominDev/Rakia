#ifndef MESH_DATA_H
#define MESH_DATA_H

#include "engine/vecmath.h"

#define MESH_GROUP_NAME_MAX 64

typedef struct {
    Vec3 position;
    Vec3 normal;
    float u;
    float v;
    Vec3 tangent;
    // +1 or -1: bitangent = cross(normal, tangent) * tangent_sign, the flip mirrored uv islands need
    float tangent_sign;
} MeshVertex;

typedef struct {
    char name[MESH_GROUP_NAME_MAX];
    int material;
    int first_index;
    int index_count;
} MeshGroup;

typedef struct {
    MeshVertex *vertices;
    unsigned int *indices;
    MeshGroup *groups;
    int vertex_count;
    int index_count;
    int group_count;
} MeshData;

// edge 1, centered on the origin, four vertices per face
int mesh_data_cube(MeshData *data);
// a square of edge size in the xz plane at y = 0, facing +y, with one texture tile every tile_size units
int mesh_data_quad(MeshData *data, float size, float tile_size);
void mesh_data_free(MeshData *data);

#endif
