#include "engine/mesh_data.h"

#include <stdlib.h>

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

int mesh_data_cube(MeshData *data)
{
    int face;

    data->vertices = malloc(sizeof *data->vertices * CUBE_VERTICES);
    data->indices = malloc(sizeof *data->indices * CUBE_INDICES);
    data->vertex_count = CUBE_VERTICES;
    data->index_count = CUBE_INDICES;
    if (data->vertices == NULL || data->indices == NULL) {
        mesh_data_free(data);
        return -1;
    }

    for (face = 0; face < CUBE_FACES; face++) {
        const Vec3 normal = cube_faces[face].normal;
        const Vec3 tangent = cube_faces[face].tangent;
        const Vec3 bitangent = v3_cross(normal, tangent);
        const Vec3 center = v3_scale(normal, 0.5f);
        MeshVertex *vertex = data->vertices + face * 4;
        unsigned int *index = data->indices + face * 6;
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
        }

        index[0] = (unsigned int)(face * 4);
        index[1] = (unsigned int)(face * 4 + 1);
        index[2] = (unsigned int)(face * 4 + 2);
        index[3] = (unsigned int)(face * 4);
        index[4] = (unsigned int)(face * 4 + 2);
        index[5] = (unsigned int)(face * 4 + 3);
    }

    return 0;
}

void mesh_data_free(MeshData *data)
{
    free(data->vertices);
    free(data->indices);
    data->vertices = NULL;
    data->indices = NULL;
    data->vertex_count = 0;
    data->index_count = 0;
}
