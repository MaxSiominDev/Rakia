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

// how many sides a tube is drawn with, and the room one of them needs
#define MESH_DATA_TUBE_SIDES 12
#define MESH_DATA_TUBE_VERTICES (4 * MESH_DATA_TUBE_SIDES + 2)
#define MESH_DATA_TUBE_INDICES (12 * MESH_DATA_TUBE_SIDES)

// room for a mesh the caller fills in itself; the groups start empty and unnamed
int mesh_data_reserve(MeshData *data, int vertex_count, int index_count, int group_count);
// one flat rectangle written into a slot of a mesh reserved for several, facing normal with right along its u
void mesh_data_rect(MeshData *data, int slot, Vec3 center, Vec3 normal, Vec3 right, float half_u, float half_v);
// one closed tube from a to b, written into a slot the same way
void mesh_data_tube(MeshData *data, int slot, Vec3 from, Vec3 to, float radius);
// edge 1, centered on the origin, four vertices per face
int mesh_data_cube(MeshData *data);
// a rectangle in the xz plane at y = 0, facing +y, with one texture tile every tile_size units
int mesh_data_quad(MeshData *data, float size_x, float size_z, float tile_size);
// standing on the xz plane with its tip at height, drawn flat so the facets show
int mesh_data_cone(MeshData *data, float radius, float height, int sides);
// the box around the vertices one group uses, for parts a placement has to be measured from
void mesh_data_group_bounds(const MeshData *data, int group, Vec3 *min, Vec3 *max);
void mesh_data_free(MeshData *data);

#endif
