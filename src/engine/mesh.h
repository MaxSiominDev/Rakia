#ifndef MESH_H
#define MESH_H

#include "engine/gl_compat.h"
#include "engine/mesh_data.h"

typedef struct {
    GLuint vertex_buffer;
    GLuint index_buffer;
    GLsizei index_count;
} Mesh;

void mesh_create(Mesh *mesh, const MeshData *data);
void mesh_draw(const Mesh *mesh);

#endif
