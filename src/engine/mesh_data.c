#include "engine/mesh_data.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CUBE_FACES 6
#define CUBE_VERTICES (CUBE_FACES * 4)
#define CUBE_INDICES (CUBE_FACES * 6)

static const struct {
    Vec3 normal;
    Vec3 tangent;
} cube_faces[CUBE_FACES] = {
    {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
    {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}}
};

static const float corner_uvs[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

static int allocate(MeshData *data, int vertex_count, int index_count)
{
    memset(data, 0, sizeof *data);
    data->vertices = malloc(sizeof *data->vertices * (size_t)vertex_count);
    data->indices = malloc(sizeof *data->indices * (size_t)index_count);
    data->groups = calloc(1, sizeof *data->groups);
    if (data->vertices == NULL || data->indices == NULL || data->groups == NULL) {
        mesh_data_free(data);
        return -1;
    }

    data->vertex_count = vertex_count;
    data->index_count = index_count;
    data->group_count = 1;
    data->groups[0].index_count = index_count;

    return 0;
}

static void quad_indices(unsigned int *index, unsigned int first_vertex)
{
    index[0] = first_vertex;
    index[1] = first_vertex + 1;
    index[2] = first_vertex + 2;
    index[3] = first_vertex;
    index[4] = first_vertex + 2;
    index[5] = first_vertex + 3;
}

int mesh_data_cube(MeshData *data)
{
    int face;

    if (allocate(data, CUBE_VERTICES, CUBE_INDICES) != 0) {
        return -1;
    }

    for (face = 0; face < CUBE_FACES; face++) {
        const Vec3 normal = cube_faces[face].normal;
        const Vec3 tangent = cube_faces[face].tangent;
        const Vec3 bitangent = v3_cross(normal, tangent);
        const Vec3 center = v3_scale(normal, 0.5f);
        MeshVertex *vertex = data->vertices + face * 4;
        int corner;

        for (corner = 0; corner < 4; corner++) {
            const float u = corner_uvs[corner][0];
            const float v = corner_uvs[corner][1];

            vertex[corner].position = v3_add(center, v3_add(v3_scale(tangent, u - 0.5f),
                                                            v3_scale(bitangent, v - 0.5f)));
            vertex[corner].normal = normal;
            vertex[corner].u = u;
            vertex[corner].v = v;
            vertex[corner].tangent = tangent;
            vertex[corner].tangent_sign = 1.0f;
        }
        quad_indices(data->indices + face * 6, (unsigned int)(face * 4));
    }

    return 0;
}

int mesh_data_quad(MeshData *data, float size, float tile_size)
{
    const float half = size * 0.5f;
    const float tiles = size / tile_size;
    int corner;

    if (allocate(data, 4, 6) != 0) {
        return -1;
    }

    for (corner = 0; corner < 4; corner++) {
        const float u = corner_uvs[corner][0];
        const float v = corner_uvs[corner][1];
        MeshVertex *vertex = data->vertices + corner;

        // with v growing toward -z, the tangent (+x) and bitangent (-z) frame stays right-handed under the +y normal
        vertex->position = v3(-half + u * size, 0.0f, half - v * size);
        vertex->normal = v3(0.0f, 1.0f, 0.0f);
        vertex->u = u * tiles;
        vertex->v = v * tiles;
        vertex->tangent = v3(1.0f, 0.0f, 0.0f);
        vertex->tangent_sign = 1.0f;
    }
    quad_indices(data->indices, 0);

    return 0;
}

void mesh_data_group_bounds(const MeshData *data, int group, Vec3 *min, Vec3 *max)
{
    const MeshGroup *entry = &data->groups[group];
    int i;

    *min = data->vertices[data->indices[entry->first_index]].position;
    *max = *min;
    for (i = 1; i < entry->index_count; i++) {
        const Vec3 p = data->vertices[data->indices[entry->first_index + i]].position;

        *min = v3(fminf(min->x, p.x), fminf(min->y, p.y), fminf(min->z, p.z));
        *max = v3(fmaxf(max->x, p.x), fmaxf(max->y, p.y), fmaxf(max->z, p.z));
    }
}

void mesh_data_free(MeshData *data)
{
    free(data->vertices);
    free(data->indices);
    free(data->groups);
    memset(data, 0, sizeof *data);
}
