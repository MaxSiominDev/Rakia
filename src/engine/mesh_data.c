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

static void quad_indices(unsigned int *index, unsigned int first_vertex)
{
    index[0] = first_vertex;
    index[1] = first_vertex + 1;
    index[2] = first_vertex + 2;
    index[3] = first_vertex;
    index[4] = first_vertex + 2;
    index[5] = first_vertex + 3;
}

int mesh_data_reserve(MeshData *data, int vertex_count, int index_count, int group_count)
{
    memset(data, 0, sizeof *data);
    data->vertices = malloc(sizeof *data->vertices * (size_t)vertex_count);
    data->indices = malloc(sizeof *data->indices * (size_t)index_count);
    data->groups = calloc((size_t)group_count, sizeof *data->groups);
    if (data->vertices == NULL || data->indices == NULL || data->groups == NULL) {
        mesh_data_free(data);
        return -1;
    }

    data->vertex_count = vertex_count;
    data->index_count = index_count;
    data->group_count = group_count;

    return 0;
}

int mesh_data_cube(MeshData *data)
{
    int face;

    if (mesh_data_reserve(data, CUBE_VERTICES, CUBE_INDICES, 1) != 0) {
        return -1;
    }
    data->groups[0].index_count = CUBE_INDICES;

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

int mesh_data_quad(MeshData *data, float size_x, float size_z, float tile_size)
{
    const float half_x = size_x * 0.5f;
    const float half_z = size_z * 0.5f;
    int corner;

    if (mesh_data_reserve(data, 4, 6, 1) != 0) {
        return -1;
    }
    data->groups[0].index_count = 6;

    for (corner = 0; corner < 4; corner++) {
        const float u = corner_uvs[corner][0];
        const float v = corner_uvs[corner][1];
        MeshVertex *vertex = data->vertices + corner;

        // with v growing toward -z, the tangent (+x) and bitangent (-z) frame stays right-handed under the +y normal
        vertex->position = v3(-half_x + u * size_x, 0.0f, half_z - v * size_z);
        vertex->normal = v3(0.0f, 1.0f, 0.0f);
        vertex->u = u * size_x / tile_size;
        vertex->v = v * size_z / tile_size;
        vertex->tangent = v3(1.0f, 0.0f, 0.0f);
        vertex->tangent_sign = 1.0f;
    }
    quad_indices(data->indices, 0);

    return 0;
}

static void write_vertex(MeshVertex *vertex, Vec3 position, Vec3 normal, Vec3 tangent, float u, float v)
{
    vertex->position = position;
    vertex->normal = normal;
    vertex->u = u;
    vertex->v = v;
    vertex->tangent = tangent;
    vertex->tangent_sign = 1.0f;
}

// a flat triangle whose tangent runs along its third edge, so a facet keeps the +u direction the other
// builders give their vertices
static void face_frame(MeshVertex *facet)
{
    const Vec3 across = v3_sub(facet[2].position, facet[0].position);
    const Vec3 normal = v3_normalize(v3_cross(v3_sub(facet[1].position, facet[0].position), across));
    const Vec3 tangent = v3_normalize(v3_sub(across, v3_scale(normal, v3_dot(normal, across))));
    int corner;

    for (corner = 0; corner < 3; corner++) {
        facet[corner].normal = normal;
        facet[corner].tangent = tangent;
        facet[corner].tangent_sign = 1.0f;
    }
}

int mesh_data_cone(MeshData *data, float radius, float height, int sides)
{
    int side;

    if (mesh_data_reserve(data, 4 * sides + 1, 6 * sides, 1) != 0) {
        return -1;
    }
    data->groups[0].index_count = 6 * sides;

    for (side = 0; side < sides; side++) {
        const float from = 2.0f * VEC_PI * (float)side / (float)sides;
        const float to = 2.0f * VEC_PI * (float)(side + 1) / (float)sides;
        const Vec3 near_corner = v3(radius * cosf(from), 0.0f, radius * sinf(from));
        const Vec3 far_corner = v3(radius * cosf(to), 0.0f, radius * sinf(to));
        MeshVertex *facet = data->vertices + side * 3;
        MeshVertex *rim = data->vertices + sides * 3 + side;
        unsigned int *facet_index = data->indices + side * 3;
        unsigned int *cap_index = data->indices + sides * 3 + side * 3;
        int i;

        facet[0].position = near_corner;
        facet[1].position = v3(0.0f, height, 0.0f);
        facet[2].position = far_corner;
        facet[0].u = (float)side / (float)sides;
        facet[1].u = ((float)side + 0.5f) / (float)sides;
        facet[2].u = (float)(side + 1) / (float)sides;
        facet[0].v = 0.0f;
        facet[1].v = 1.0f;
        facet[2].v = 0.0f;
        face_frame(facet);

        write_vertex(rim, near_corner, v3(0.0f, -1.0f, 0.0f), v3(1.0f, 0.0f, 0.0f), 0.5f + 0.5f * cosf(from),
                     0.5f + 0.5f * sinf(from));

        for (i = 0; i < 3; i++) {
            facet_index[i] = (unsigned int)(side * 3 + i);
        }
        cap_index[0] = (unsigned int)(sides * 4);
        cap_index[1] = (unsigned int)(sides * 3 + side);
        cap_index[2] = (unsigned int)(sides * 3 + (side + 1) % sides);
    }

    write_vertex(data->vertices + sides * 4, v3(0.0f, 0.0f, 0.0f), v3(0.0f, -1.0f, 0.0f), v3(1.0f, 0.0f, 0.0f),
                 0.5f, 0.5f);

    return 0;
}

void mesh_data_rect(MeshData *data, int slot, Vec3 center, Vec3 normal, Vec3 right, float half_u, float half_v)
{
    static const float corners[4][2] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
    const Vec3 up = v3_cross(normal, right);
    MeshVertex *vertex = data->vertices + slot * 4;
    int corner;

    for (corner = 0; corner < 4; corner++) {
        const float u = corners[corner][0];
        const float v = corners[corner][1];

        write_vertex(vertex + corner, v3_add(center, v3_add(v3_scale(right, u * half_u), v3_scale(up, v * half_v))),
                     normal, right, 0.5f + 0.5f * u, 0.5f + 0.5f * v);
    }
    quad_indices(data->indices + slot * 6, (unsigned int)(slot * 4));
}

void mesh_data_tube(MeshData *data, int slot, Vec3 from, Vec3 to, float radius)
{
    const int sides = MESH_DATA_TUBE_SIDES;
    const Vec3 axis = v3_normalize(v3_sub(to, from));
    // any direction across the axis will do for the seam, as long as it is not the axis itself
    const Vec3 across = fabsf(axis.y) < 0.9f ? v3(0.0f, 1.0f, 0.0f) : v3(1.0f, 0.0f, 0.0f);
    const Vec3 aside = v3_normalize(v3_cross(across, axis));
    const Vec3 up = v3_cross(axis, aside);
    const unsigned int base = (unsigned int)(slot * MESH_DATA_TUBE_VERTICES);
    MeshVertex *vertex = data->vertices + slot * MESH_DATA_TUBE_VERTICES;
    unsigned int *index = data->indices + slot * MESH_DATA_TUBE_INDICES;
    int side;

    write_vertex(vertex + 4 * sides, from, v3_scale(axis, -1.0f), aside, 0.5f, 0.5f);
    write_vertex(vertex + 4 * sides + 1, to, axis, aside, 0.5f, 0.5f);

    for (side = 0; side < sides; side++) {
        const float angle = 2.0f * VEC_PI * (float)side / (float)sides;
        const Vec3 out = v3_add(v3_scale(aside, cosf(angle)), v3_scale(up, sinf(angle)));
        const Vec3 rim = v3_scale(out, radius);
        const Vec3 along = v3_cross(axis, out);
        const float cap_u = 0.5f + 0.5f * cosf(angle);
        const float cap_v = 0.5f + 0.5f * sinf(angle);
        const unsigned int near_wall = base + (unsigned int)side;
        const unsigned int far_wall = near_wall + sides;
        const unsigned int next = base + (unsigned int)((side + 1) % sides);

        write_vertex(vertex + side, v3_add(from, rim), out, along, (float)side / (float)sides, 0.0f);
        write_vertex(vertex + sides + side, v3_add(to, rim), out, along, (float)side / (float)sides, 1.0f);
        write_vertex(vertex + 2 * sides + side, v3_add(from, rim), v3_scale(axis, -1.0f), aside, cap_u, cap_v);
        write_vertex(vertex + 3 * sides + side, v3_add(to, rim), axis, aside, cap_u, cap_v);

        index[side * 6] = near_wall;
        index[side * 6 + 1] = next;
        index[side * 6 + 2] = far_wall;
        index[side * 6 + 3] = next;
        index[side * 6 + 4] = next + sides;
        index[side * 6 + 5] = far_wall;

        index[sides * 6 + side * 3] = base + 4 * sides;
        index[sides * 6 + side * 3 + 1] = next + 2 * sides;
        index[sides * 6 + side * 3 + 2] = near_wall + 2 * sides;

        index[sides * 9 + side * 3] = base + 4 * sides + 1;
        index[sides * 9 + side * 3 + 1] = near_wall + 3 * sides;
        index[sides * 9 + side * 3 + 2] = next + 3 * sides;
    }
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
