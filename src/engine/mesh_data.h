#ifndef MESH_DATA_H
#define MESH_DATA_H

#include "engine/vecmath.h"

typedef struct {
    Vec3 position;
    Vec3 normal;
    float u;
    float v;
    Vec3 tangent;
} MeshVertex;

typedef struct {
    MeshVertex *vertices;
    unsigned int *indices;
    int vertex_count;
    int index_count;
} MeshData;

// edge 1, centered on the origin, four vertices per face
int mesh_data_cube(MeshData *data);
void mesh_data_free(MeshData *data);

#endif
