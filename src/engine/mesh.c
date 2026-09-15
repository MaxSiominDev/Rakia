#include "engine/mesh.h"

#include "engine/gl_ext.h"
#include "engine/shader.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void measure_bounds(Mesh *mesh, const MeshData *data)
{
    Vec3 min = data->vertex_count > 0 ? data->vertices[0].position : v3(0.0f, 0.0f, 0.0f);
    Vec3 max = min;
    int i;

    for (i = 1; i < data->vertex_count; i++) {
        const Vec3 p = data->vertices[i].position;

        min = v3(fminf(min.x, p.x), fminf(min.y, p.y), fminf(min.z, p.z));
        max = v3(fmaxf(max.x, p.x), fmaxf(max.y, p.y), fmaxf(max.z, p.z));
    }
    mesh->bounds_min = min;
    mesh->bounds_max = max;
}

int mesh_create(Mesh *mesh, const MeshData *data)
{
    const size_t groups_size = sizeof *data->groups * (size_t)data->group_count;

    mesh->groups = malloc(groups_size);
    if (mesh->groups == NULL) {
        return -1;
    }
    memcpy(mesh->groups, data->groups, groups_size);
    mesh->group_count = data->group_count;
    measure_bounds(mesh, data);

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

    return 0;
}

int mesh_group_index(const Mesh *mesh, const char *name)
{
    int i;

    for (i = 0; i < mesh->group_count; i++) {
        if (strcmp(mesh->groups[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static void point_attribute(GLuint index, GLint components, size_t offset)
{
    glVertexAttribPointer(index, components, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (const void *)(uintptr_t)offset);
    glEnableVertexAttribArray(index);
}

void mesh_bind(const Mesh *mesh)
{
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);

    point_attribute(SHADER_ATTRIBUTE_POSITION, 3, offsetof(MeshVertex, position));
    point_attribute(SHADER_ATTRIBUTE_NORMAL, 3, offsetof(MeshVertex, normal));
    point_attribute(SHADER_ATTRIBUTE_UV, 2, offsetof(MeshVertex, u));
    // the sign after the tangent rides along as the fourth component
    point_attribute(SHADER_ATTRIBUTE_TANGENT, 4, offsetof(MeshVertex, tangent));
}

void mesh_draw_group(const Mesh *mesh, int group)
{
    const MeshGroup *g = &mesh->groups[group];

    glDrawElements(GL_TRIANGLES, g->index_count, GL_UNSIGNED_INT,
                   (const void *)(uintptr_t)(sizeof(unsigned int) * (size_t)g->first_index));
}

void mesh_unbind(void)
{
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_TANGENT);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_UV);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_NORMAL);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void mesh_free(Mesh *mesh)
{
    glDeleteBuffers(1, &mesh->vertex_buffer);
    glDeleteBuffers(1, &mesh->index_buffer);
    free(mesh->groups);
    memset(mesh, 0, sizeof *mesh);
}
