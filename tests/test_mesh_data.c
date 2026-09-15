#include "check.h"
#include "engine/mesh_data.h"
#include "engine/vecmath.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static void check_frames(const MeshData *data, const char *what)
{
    int i;

    for (i = 0; i < data->vertex_count; i++) {
        const MeshVertex *v = &data->vertices[i];

        check_close(v3_length(v->normal), 1.0f, 1e-6f, what);
        check_close(v3_length(v->tangent), 1.0f, 1e-6f, what);
        check_close(v3_dot(v->normal, v->tangent), 0.0f, 1e-6f, what);
        check(v->tangent_sign == 1.0f, what);
    }
}

static void check_winding(const MeshData *data, const char *what)
{
    int i;

    for (i = 0; i < data->index_count; i += 3) {
        const unsigned int a = data->indices[i];
        const unsigned int b = data->indices[i + 1];
        const unsigned int c = data->indices[i + 2];
        Vec3 winding;

        if (a >= (unsigned int)data->vertex_count || b >= (unsigned int)data->vertex_count ||
            c >= (unsigned int)data->vertex_count) {
            check(0, what);
            continue;
        }
        winding = v3_cross(v3_sub(data->vertices[b].position, data->vertices[a].position),
                           v3_sub(data->vertices[c].position, data->vertices[a].position));
        check(v3_dot(winding, data->vertices[a].normal) > 0.0f, what);
    }
}

static void test_cube(void)
{
    MeshData cube;
    int i;

    check(mesh_data_cube(&cube) == 0, "cube builds");
    check(cube.vertex_count == 24, "cube has 24 vertices");
    check(cube.index_count == 36, "cube has 36 indices");
    check(cube.group_count == 1 && cube.groups[0].first_index == 0 && cube.groups[0].index_count == 36,
          "cube is one group over all indices");
    check(cube.groups[0].name[0] == '\0' && cube.groups[0].material == 0, "cube group is unnamed with material 0");

    for (i = 0; i < cube.vertex_count; i++) {
        const MeshVertex *v = &cube.vertices[i];

        check(fabsf(v->position.x) <= 0.5f && fabsf(v->position.y) <= 0.5f && fabsf(v->position.z) <= 0.5f,
              "vertex lies on the unit cube");
        check(v3_dot(v->normal, v->position) > 0.49f, "normal points outward");
        check((v->u == 0.0f || v->u == 1.0f) && (v->v == 0.0f || v->v == 1.0f), "uv is a corner");
    }
    check_frames(&cube, "cube vertex frame is orthonormal");
    check_winding(&cube, "cube triangle winds counter-clockwise from outside");

    mesh_data_free(&cube);
    check(cube.vertices == NULL && cube.groups == NULL && cube.index_count == 0, "free clears the data");
}

static void test_quad(void)
{
    MeshData quad;
    int i;

    check(mesh_data_quad(&quad, 200.0f, 1.8f) == 0, "quad builds");
    check(quad.vertex_count == 4 && quad.index_count == 6, "quad has four vertices and two triangles");
    check(quad.group_count == 1 && quad.groups[0].index_count == 6, "quad is one group");

    for (i = 0; i < quad.vertex_count; i++) {
        const MeshVertex *v = &quad.vertices[i];

        check(fabsf(v->position.x) == 100.0f && fabsf(v->position.z) == 100.0f && v->position.y == 0.0f,
              "quad corner lies on the 200 m square at y = 0");
        check(v->normal.y == 1.0f, "quad faces up");
        check(v->tangent.x == 1.0f, "quad tangent runs along +x");
        check_close(v->u, (v->position.x + 100.0f) / 1.8f, 1e-3f, "u counts tiles from the -x edge");
        check_close(v->v, (100.0f - v->position.z) / 1.8f, 1e-3f, "v counts tiles from the +z edge");
    }
    check_frames(&quad, "quad vertex frame is orthonormal");
    check_winding(&quad, "quad triangles wind counter-clockwise seen from above");

    mesh_data_free(&quad);
}

static void test_group_bounds(void)
{
    MeshVertex vertices[4];
    unsigned int indices[6] = {0, 1, 2, 1, 2, 3};
    MeshGroup groups[2] = {{"front", 0, 0, 3}, {"back", 0, 3, 3}};
    MeshData data;
    Vec3 min;
    Vec3 max;

    memset(vertices, 0, sizeof vertices);
    vertices[0].position = v3(-1.0f, 0.0f, 2.0f);
    vertices[1].position = v3(3.0f, 1.0f, -4.0f);
    vertices[2].position = v3(0.0f, -2.0f, 0.0f);
    vertices[3].position = v3(5.0f, 7.0f, 6.0f);
    data.vertices = vertices;
    data.indices = indices;
    data.groups = groups;
    data.vertex_count = 4;
    data.index_count = 6;
    data.group_count = 2;

    mesh_data_group_bounds(&data, 0, &min, &max);
    check_v3(min, -1.0f, -2.0f, -4.0f, "a group's box holds the lowest corner it uses");
    check_v3(max, 3.0f, 1.0f, 2.0f, "and the highest");

    mesh_data_group_bounds(&data, 1, &min, &max);
    check_v3(min, 0.0f, -2.0f, -4.0f, "another group measures only its own vertices");
    check_v3(max, 5.0f, 7.0f, 6.0f, "on both sides");
}

void test_mesh_data_main(void)
{
    test_cube();
    test_quad();
    test_group_bounds();
}
