#ifndef MESH_H
#define MESH_H

#include "engine/gl_compat.h"
#include "engine/mesh_data.h"

typedef struct {
    GLuint vertex_buffer;
    GLuint index_buffer;
    MeshGroup *groups;
    int group_count;
    Vec3 bounds_min;
    Vec3 bounds_max;
} Mesh;

int mesh_create(Mesh *mesh, const MeshData *data);
// -1 when no group has that name
int mesh_group_index(const Mesh *mesh, const char *name);
void mesh_bind(const Mesh *mesh);
void mesh_draw_group(const Mesh *mesh, int group);
void mesh_unbind(void);
void mesh_free(Mesh *mesh);

#endif
