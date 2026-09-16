#include "check.h"
#include "engine/terrain.h"
#include "game/targets.h"

#include <math.h>
#include <string.h>

#define SEED_A 4242u
#define SEED_B 777u

static float circular_gap(float a, float b)
{
    return fabsf(atan2f(sinf(a - b), cosf(a - b)));
}

static void test_layout_bounds(void)
{
    TargetSpot spots[TARGETS_COUNT];
    int i;

    targets_layout(SEED_A, spots);
    for (i = 0; i < TARGETS_COUNT; i++) {
        const float range = v3_length(v3(spots[i].position.x, 0.0f, spots[i].position.z));

        check(range >= TARGETS_NEAR - 1.0f && range <= TARGETS_FAR + 1.0f,
              "every target stands between three and ten kilometres out");
        check(!terrain_on_base(spots[i].position.x, spots[i].position.z), "clear of the flattened airbase patch");
        check(spots[i].position.y > TERRAIN_SEA_LEVEL, "and on dry land, not under the sea");
    }
}

static void test_layout_spread(void)
{
    TargetSpot spots[TARGETS_COUNT];
    float widest = 0.0f;
    int i;
    int j;

    targets_layout(SEED_A, spots);
    for (i = 0; i < TARGETS_COUNT; i++) {
        for (j = i + 1; j < TARGETS_COUNT; j++) {
            const float angle_i = atan2f(spots[i].position.x, spots[i].position.z);
            const float angle_j = atan2f(spots[j].position.x, spots[j].position.z);

            widest = fmaxf(widest, circular_gap(angle_i, angle_j));
        }
    }
    check(widest > 90.0f * VEC_DEGREES, "some pair of targets stands well apart in direction, not bunched together");
}

static void test_layout_spacing(void)
{
    TargetSpot spots[TARGETS_COUNT];
    int i;
    int j;

    targets_layout(SEED_A, spots);
    for (i = 0; i < TARGETS_COUNT; i++) {
        for (j = i + 1; j < TARGETS_COUNT; j++) {
            const float apart = v3_length(v3_sub(spots[i].position, spots[j].position));

            check(apart >= TARGETS_SPACING - 1.0f, "no two targets stand closer than a kilometre apart");
        }
    }
}

static void test_layout_deterministic(void)
{
    TargetSpot a[TARGETS_COUNT];
    TargetSpot b[TARGETS_COUNT];
    TargetSpot c[TARGETS_COUNT];
    int i;
    int differs = 0;

    targets_layout(SEED_A, a);
    targets_layout(SEED_A, b);
    targets_layout(SEED_B, c);

    for (i = 0; i < TARGETS_COUNT; i++) {
        check_v3(b[i].position, a[i].position.x, a[i].position.y, a[i].position.z,
                "the same seed lays the targets out the same way");
        check_close(b[i].yaw, a[i].yaw, 1e-6f, "with the same yaw too");
        differs = differs || v3_length(v3_sub(c[i].position, a[i].position)) > 1.0f;
    }
    check(differs, "a different seed lays them out differently");
}

static void test_hitbox_transform(void)
{
    Target target;
    Hitbox box;

    memset(&target, 0, sizeof target);
    target.position = v3(100.0f, 5.0f, -200.0f);
    target.yaw = VEC_PI * 0.5f;
    target.min = v3(-2.0f, 0.0f, -3.0f);
    target.max = v3(2.0f, 4.0f, 3.0f);

    box = target_hitbox(&target);
    check_v3(box.position, 100.0f, 5.0f, -200.0f, "the hitbox sits at the target's own position");
    check_close(box.yaw, VEC_PI * 0.5f, 1e-6f, "turned by the target's own yaw");
    check_v3(box.min, -2.0f, 0.0f, -3.0f, "with the box it was given, unrotated in its own frame");
    check_v3(box.max, 2.0f, 4.0f, 3.0f, "on both sides");
}

static void test_hit_and_destroy(void)
{
    Targets targets;

    memset(&targets, 0, sizeof targets);
    targets.list[0].position = v3(0.0f, 0.0f, 100.0f);
    targets.list[0].min = v3(-5.0f, 0.0f, -5.0f);
    targets.list[0].max = v3(5.0f, 5.0f, 5.0f);
    targets.list[0].alive = 1;

    check(targets_hit(&targets, v3(0.0f, 2.0f, 0.0f), v3(0.0f, 2.0f, 200.0f), 0.5f) == 0,
          "a path straight through the box hits the target");
    check(targets_hit(&targets, v3(50.0f, 2.0f, 0.0f), v3(50.0f, 2.0f, 200.0f), 0.5f) < 0,
          "a path well clear of the box misses");

    targets_destroy(&targets, 0);
    check(!targets.list[0].alive, "destroying it clears alive");
    check(targets.list[0].removing > 0.0f, "and starts the removal countdown");
    check(targets_hit(&targets, v3(0.0f, 2.0f, 0.0f), v3(0.0f, 2.0f, 200.0f), 0.5f) < 0,
          "a destroyed target answers no more hits");
}

static void test_nearest_and_next(void)
{
    Targets targets;
    int i;

    memset(&targets, 0, sizeof targets);
    for (i = 0; i < TARGETS_COUNT; i++) {
        targets.list[i].position = v3((float)i * 1000.0f, 0.0f, 0.0f);
        targets.list[i].alive = 1;
    }
    check(targets_alive(&targets) == TARGETS_COUNT, "every target starts alive");
    check(targets_nearest(&targets, v3(2100.0f, 0.0f, 0.0f)) == 2, "the nearest live target to a point");

    targets.list[2].alive = 0;
    check(targets_alive(&targets) == TARGETS_COUNT - 1, "destroying one drops the live count");
    check(targets_nearest(&targets, v3(2100.0f, 0.0f, 0.0f)) == 3, "a dead one does not count, so the next nearest wins");

    check(targets_next(&targets, 1) == 3, "next skips over a dead target");
    check(targets_next(&targets, 4) == 0, "and wraps back around to the first live one");

    for (i = 0; i < TARGETS_COUNT; i++) {
        targets.list[i].alive = 0;
    }
    check(targets_alive(&targets) == 0, "none left alive");
    check(targets_nearest(&targets, v3(0.0f, 0.0f, 0.0f)) == -1, "nearest finds nothing");
    check(targets_next(&targets, 0) == -1, "and next finds nothing either");
}

void test_targets_main(void)
{
    test_layout_bounds();
    test_layout_spread();
    test_layout_spacing();
    test_layout_deterministic();
    test_hitbox_transform();
    test_hit_and_destroy();
    test_nearest_and_next();
}
