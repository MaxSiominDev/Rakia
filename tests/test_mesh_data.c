#include "check.h"
#include "engine/mesh_data.h"
#include "engine/vecmath.h"

#include <math.h>
#include <stddef.h>

void test_mesh_data_main(void)
{
    MeshData cube;
    int i;

    check(mesh_data_cube(&cube) == 0, "cube builds");
    check(cube.vertex_count == 24, "cube has 24 vertices");
    check(cube.index_count == 36, "cube has 36 indices");

    for (i = 0; i < cube.vertex_count; i++) {
        const MeshVertex *v = &cube.vertices[i];

        check(fabsf(v->position.x) <= 0.5f && fabsf(v->position.y) <= 0.5f && fabsf(v->position.z) <= 0.5f,
              "vertex lies on the unit cube");
        check_close(v3_length(v->normal), 1.0f, 1e-6f, "normal is unit");
        check(v3_dot(v->normal, v->position) > 0.49f, "normal points outward");
        check_close(v3_length(v->tangent), 1.0f, 1e-6f, "tangent is unit");
        check_close(v3_dot(v->normal, v->tangent), 0.0f, 1e-6f, "tangent is orthogonal to the normal");
        check((v->u == 0.0f || v->u == 1.0f) && (v->v == 0.0f || v->v == 1.0f), "uv is a corner");
    }

    for (i = 0; i < cube.index_count; i += 3) {
        const unsigned int a = cube.indices[i];
        const unsigned int b = cube.indices[i + 1];
        const unsigned int c = cube.indices[i + 2];
        Vec3 winding;

        if (a >= 24 || b >= 24 || c >= 24) {
            check(0, "index in range");
            continue;
        }
        winding = v3_cross(v3_sub(cube.vertices[b].position, cube.vertices[a].position),
                           v3_sub(cube.vertices[c].position, cube.vertices[a].position));
        check(v3_dot(winding, cube.vertices[a].normal) > 0.0f, "triangle winds counter-clockwise from outside");
        check_close(v3_length(winding), 1.0f, 1e-6f, "triangle edges span a unit face");
    }

    mesh_data_free(&cube);
    check(cube.vertices == NULL && cube.index_count == 0, "free clears the data");
}
