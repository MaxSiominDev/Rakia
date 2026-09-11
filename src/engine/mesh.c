#include "engine/mesh.h"

#include "engine/gl_ext.h"
#include "engine/shader.h"

#include <stddef.h>
#include <stdint.h>

void mesh_create(Mesh *mesh, const MeshData *data)
{
    glGenBuffers(1, &mesh->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof *data->vertices * (size_t)data->vertex_count),
                 data->vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &mesh->index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof *data->indices * (size_t)data->index_count),
                 data->indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mesh->index_count = data->index_count;
}

static void point_attribute(GLuint index, GLint components, size_t offset)
{
    glVertexAttribPointer(index, components, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (const void *)(uintptr_t)offset);
    glEnableVertexAttribArray(index);
}

void mesh_draw(const Mesh *mesh)
{
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);

    point_attribute(SHADER_ATTRIBUTE_POSITION, 3, offsetof(MeshVertex, position));
    point_attribute(SHADER_ATTRIBUTE_NORMAL, 3, offsetof(MeshVertex, normal));
    point_attribute(SHADER_ATTRIBUTE_UV, 2, offsetof(MeshVertex, u));
    point_attribute(SHADER_ATTRIBUTE_TANGENT, 3, offsetof(MeshVertex, tangent));

    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(SHADER_ATTRIBUTE_TANGENT);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_UV);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_NORMAL);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
