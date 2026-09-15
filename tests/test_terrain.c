#include "check.h"
#include "engine/terrain.h"
#include "engine/vecmath.h"

#include <math.h>

#define FAR_WEST 9000.0f
#define FAR_NORTH 14000.0f
#define FAR_SOUTH (-14000.0f)

static void test_base(void)
{
    int flat = 1;
    int i;

    for (i = 0; i < 64; i++) {
        const float angle = (float)i * 0.1f;
        const float part = (float)i / 64.0f;
        const float x = TERRAIN_BASE_HALF_WIDTH * part * cosf(angle);
        const float z = TERRAIN_BASE_HALF_LENGTH * part * sinf(angle);

        flat &= terrain_on_base(x, z) && terrain_height(x, z) == 0.0f;
    }

    check(flat, "the airbase patch is flat at zero");
    check(terrain_height(0.0f, TERRAIN_BASE_HALF_LENGTH) == 0.0f, "and reaches the far end of the runway");
    check_v3(terrain_normal(0.0f, 0.0f), 0.0f, 1.0f, 0.0f, "the base is level");
    check(!terrain_on_base(0.0f, TERRAIN_BASE_HALF_LENGTH + 1.0f), "the patch ends where the runway does");
    check(terrain_height(0.0f, 6000.0f) != 0.0f, "the land outside the patch is shaped");
}

static void test_repeatable(void)
{
    int same = 1;
    int i;

    for (i = 0; i < 200; i++) {
        const float x = -6000.0f + (float)i * 71.3f;
        const float z = 4000.0f - (float)i * 53.7f;
        const TerrainPoint point = terrain_sample(x, z);

        same &= terrain_sample(x, z).height == point.height && terrain_height(x, z) == point.height;
    }

    check(same, "the same place always gives the same height");
}

// the last column or row of one chunk against the first of its neighbor
static int shared_edge_matches(int level, int step_x, int step_z)
{
    const int side = TERRAIN_CHUNK_QUADS + 1;
    MeshData near_chunk;
    MeshData far_chunk;
    int same = 1;
    int k;

    if (terrain_chunk_data(&near_chunk, level, 0, 0) != 0 ||
        terrain_chunk_data(&far_chunk, level, step_x, step_z) != 0) {
        return 0;
    }
    for (k = 0; k <= TERRAIN_CHUNK_QUADS; k++) {
        const MeshVertex *a = step_x != 0 ? &near_chunk.vertices[k * side + TERRAIN_CHUNK_QUADS]
                                          : &near_chunk.vertices[TERRAIN_CHUNK_QUADS * side + k];
        const MeshVertex *b = step_x != 0 ? &far_chunk.vertices[k * side] : &far_chunk.vertices[k];

        same &= a->position.x == b->position.x && a->position.y == b->position.y &&
                a->position.z == b->position.z && a->normal.y == b->normal.y;
    }
    mesh_data_free(&near_chunk);
    mesh_data_free(&far_chunk);

    return same;
}

// every skirt vertex hangs straight below one of the border vertices, on all four edges
static int skirt_hangs_below(int level)
{
    const int side = TERRAIN_CHUNK_QUADS + 1;
    MeshData chunk;
    int hangs = 1;
    int k;

    if (terrain_chunk_data(&chunk, level, -2, 3) != 0) {
        return 0;
    }
    for (k = 0; k < 4 * side; k++) {
        const MeshVertex *skirt = &chunk.vertices[side * side + k];
        int matches = 0;
        int i;

        for (i = 0; i < side * side; i++) {
            const MeshVertex *surface = &chunk.vertices[i];

            if (surface->position.x == skirt->position.x && surface->position.z == skirt->position.z) {
                matches += skirt->position.y < surface->position.y;
            }
        }
        hangs &= matches > 0;
    }
    mesh_data_free(&chunk);

    return hangs;
}

static void test_chunk_borders(void)
{
    check(shared_edge_matches(0, 1, 0), "neighboring chunks share their border vertices exactly");
    check(shared_edge_matches(0, 0, 1), "along the other axis too");
    check(shared_edge_matches(2, 0, 1), "and at a coarser level");
    check(skirt_hangs_below(0), "every skirt vertex hangs below its border vertex");
    check(skirt_hangs_below(3), "at the coarsest level as well");
}

static void test_sea_and_biome(void)
{
    int drowned = 1;
    int greener = 1;
    int i;

    for (i = 0; i < 40; i++) {
        const float along = -8000.0f + (float)i * 400.0f;
        // east of the base, where the sea does not wash the biome out
        const float inland = along - 4000.0f;

        drowned &= terrain_height(FAR_WEST, along) < TERRAIN_SEA_LEVEL;
        greener &= terrain_sample(inland, FAR_NORTH).green > terrain_sample(inland, FAR_SOUTH).green;
    }

    check(drowned, "the land far to the west lies under the sea");
    check(greener, "the north is greener than the south");
    check(terrain_sample(0.0f, FAR_SOUTH).green == 0.0f, "the desert has no green at all");
    check(terrain_sample(0.0f, FAR_NORTH).green == 1.0f, "the far north is fully green");
    check(terrain_sample(FAR_WEST, 0.0f).green == 0.0f, "the sea carries no biome");
}

void test_terrain_main(void)
{
    test_base();
    test_repeatable();
    test_chunk_borders();
    test_sea_and_biome();
}
