#include "check.h"
#include "engine/collision.h"
#include "engine/vecmath.h"

#include <math.h>

#define QUARTER_TURN (VEC_PI * 0.5f)
#define EIGHTH_TURN (VEC_PI * 0.25f)

static Footprint box(float x, float z, float half_x, float half_z, float yaw)
{
    Footprint result;

    result.center = v3(x, 0.0f, z);
    result.half_x = half_x;
    result.half_z = half_z;
    result.yaw = yaw;

    return result;
}

static Footprint moved_by(Footprint footprint, Vec3 push)
{
    footprint.center = v3_add(footprint.center, push);

    return footprint;
}

static void test_footprint(void)
{
    const Vec3 min = v3(-2.0f, 0.0f, -6.0f);
    const Vec3 max = v3(2.0f, 3.0f, 4.0f);
    const Vec3 position = v3(10.0f, 0.0f, 20.0f);
    Footprint parked;

    parked = collision_footprint(position, 0.0f, min, max);
    check_v3(parked.center, 10.0f, 1.5f, 19.0f, "an unturned model carries the offset of its own box");
    check_close(parked.half_x, 2.0f, 1e-6f, "the footprint is as wide as the model");
    check_close(parked.half_z, 5.0f, 1e-6f, "and as long");

    parked = collision_footprint(position, QUARTER_TURN, min, max);
    check_v3(parked.center, 9.0f, 1.5f, 20.0f, "a quarter turn takes that offset around with the model");
    check_close(parked.yaw, QUARTER_TURN, 1e-6f, "and the footprint turns with it");
}

static void test_push(void)
{
    const Footprint a = box(0.0f, 0.0f, 2.0f, 3.0f, 0.0f);
    const Footprint b = box(3.0f, 0.0f, 2.0f, 3.0f, 0.0f);
    const Footprint touching = box(4.0f, 0.0f, 2.0f, 3.0f, 0.0f);
    const Footprint clear = box(10.0f, 0.0f, 2.0f, 3.0f, 0.0f);
    Footprint moved;
    Vec3 push;

    check(collision_push(&a, &b, &push) == 1, "two boxes over the same ground ask for a move");
    check_v3(push, -1.0f, 0.0f, 0.0f, "the short way out of the overlap, flat on the ground");
    moved = moved_by(a, push);
    check(collision_push(&moved, &b, &push) == 0, "which leaves them just touching");

    push = v3(9.0f, 9.0f, 9.0f);
    check(collision_push(&a, &touching, &push) == 0, "boxes that only touch are left where they stand");
    check(collision_push(&a, &clear, &push) == 0, "so are boxes with ground between them");
    check_v3(push, 9.0f, 9.0f, 9.0f, "and the move the caller asked about stays untouched");
}

static void test_turned(void)
{
    const Footprint square = box(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    const Footprint across = box(4.5f, 0.0f, 1.0f, 4.0f, QUARTER_TURN);
    const Footprint along = box(4.5f, 0.0f, 1.0f, 4.0f, 0.0f);
    const Footprint diagonal = box(2.2f, 0.0f, 1.0f, 1.0f, EIGHTH_TURN);
    Vec3 push;

    check(collision_push(&square, &across, &push) == 1, "a long box turned across the square reaches it");
    check_v3(push, -0.5f, 0.0f, 0.0f, "and pushes it out by what is left of that reach");
    check(collision_push(&square, &along, &push) == 0, "the same box lying the other way stays clear");

    check(collision_push(&square, &diagonal, &push) == 1, "a box turned half of that reaches by its corner");
    check_close(push.x, 2.2f - 1.0f - sqrtf(2.0f), 1e-5f, "so the move out is the diagonal minus the gap");
    check_close(push.z, 0.0f, 1e-5f, "and stays on the axis of the box it meets");
}

// the sizes off the apron: a 9.3 by 15 metre jet and a 4.1 by 0.6 metre missile
static void test_apron(void)
{
    const Footprint jet = box(0.0f, 0.0f, 4.65f, 7.5f, 0.0f);
    const Footprint missile = box(4.0f, 0.0f, 0.3f, 2.05f, 0.0f);
    Footprint moved;
    Vec3 push;

    check(collision_push(&missile, &jet, &push) == 1, "a missile left under the wing overlaps the jet");
    check_close(push.x, 0.95f, 1e-5f, "and is moved out past the wing tip");
    check_close(push.z, 0.0f, 1e-5f, "rather than the long way down the fuselage");
    moved = moved_by(missile, push);
    check(collision_push(&moved, &jet, &push) == 0, "one move is enough to leave it clear");
}

static void test_segment(void)
{
    Hitbox box;

    box.position = v3(0.0f, 0.0f, 100.0f);
    box.yaw = 0.0f;
    box.min = v3(-5.0f, -5.0f, -5.0f);
    box.max = v3(5.0f, 5.0f, 5.0f);

    check(collision_segment(&box, v3(0.0f, 0.0f, 0.0f), v3(0.0f, 0.0f, 200.0f), 0.5f) == 1,
          "a path straight through the box crosses it");
    check(collision_segment(&box, v3(50.0f, 0.0f, 0.0f), v3(50.0f, 0.0f, 200.0f), 0.5f) == 0,
          "a path well clear of the box misses it");
    check(collision_segment(&box, v3(0.0f, 0.0f, 300.0f), v3(0.0f, 0.0f, 400.0f), 0.5f) == 0,
          "a path that never reaches the box's distance misses too");
    check(collision_segment(&box, v3(0.0f, 0.0f, 100.0f), v3(0.0f, 0.0f, 100.0f), 0.5f) == 1,
          "a point already inside the box counts as a hit");
    check(collision_segment(&box, v3(7.5f, 0.0f, 0.0f), v3(7.5f, 0.0f, 200.0f), 3.0f) == 1,
          "the sweep radius reaches a few metres past the box's own face");
    check(collision_segment(&box, v3(7.5f, 0.0f, 0.0f), v3(7.5f, 0.0f, 200.0f), 1.0f) == 0,
          "but not without that extra reach");
}

void test_collision_main(void)
{
    test_footprint();
    test_push();
    test_turned();
    test_apron();
    test_segment();
}
