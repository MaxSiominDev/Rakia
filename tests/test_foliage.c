#include "check.h"
#include "engine/foliage.h"
#include "engine/terrain.h"

// cell indices: one straddling the edge of the airbase, one in the desert, one in the green north, one at sea
#define EDGE_CELL 1
#define DESERT_CELL (-3)
#define NORTH_CELL 14
#define SEA_CELL 9
#define SPRUCE 3

static FoliagePlant plants[FOLIAGE_PER_CELL];

static void test_deterministic(void)
{
    static FoliagePlant again[FOLIAGE_PER_CELL];
    const int count = foliage_place(0, NORTH_CELL, plants, FOLIAGE_PER_CELL);
    int same = 1;
    int i;

    check(count > 0 && count <= FOLIAGE_PER_CELL, "the green north is planted");
    check(foliage_place(0, NORTH_CELL, again, FOLIAGE_PER_CELL) == count, "the same cell gives the same count");
    for (i = 0; i < count; i++) {
        same &= again[i].kind == plants[i].kind && again[i].position.x == plants[i].position.x &&
                again[i].position.z == plants[i].position.z && again[i].height == plants[i].height;
    }
    check(same, "the same cell gives the same plants");
    check(foliage_place(0, NORTH_CELL, again, 5) == 5, "the room left by the caller is respected");
}

static void test_placement(void)
{
    const int count = foliage_place(EDGE_CELL, EDGE_CELL, plants, FOLIAGE_PER_CELL);
    int off_the_base = 1;
    int above_water = 1;
    int on_gentle_ground = 1;
    int standing = 1;
    int i;

    check(count > 0, "the desert beside the base is planted");
    for (i = 0; i < count; i++) {
        const Vec3 p = plants[i].position;

        off_the_base &= !terrain_on_base(p.x, p.z);
        above_water &= p.y > TERRAIN_SEA_LEVEL;
        on_gentle_ground &= terrain_normal(p.x, p.z).y >= 0.8f;
        standing &= p.y == terrain_height(p.x, p.z) && plants[i].height > 0.0f;
    }

    check(off_the_base, "nothing grows on the airbase");
    check(above_water, "nothing grows below the sea");
    check(on_gentle_ground, "nothing grows on a cliff");
    check(standing, "every plant stands on the ground");
}

static void test_biome(void)
{
    const int desert = foliage_place(DESERT_CELL, DESERT_CELL, plants, FOLIAGE_PER_CELL);
    int trees_in_the_desert = 0;
    int trees_in_the_north = 0;
    int north;
    int i;

    for (i = 0; i < desert; i++) {
        trees_in_the_desert += plants[i].kind == SPRUCE;
    }
    north = foliage_place(0, NORTH_CELL, plants, FOLIAGE_PER_CELL);
    for (i = 0; i < north; i++) {
        trees_in_the_north += plants[i].kind == SPRUCE;
    }

    check(desert > 0 && trees_in_the_desert == 0, "the desert grows no conifers");
    check(trees_in_the_north > 0, "the green north does");
    check(foliage_place(SEA_CELL, 0, plants, FOLIAGE_PER_CELL) == 0, "the sea grows nothing");
}

void test_foliage_main(void)
{
    test_deterministic();
    test_placement();
    test_biome();
}
