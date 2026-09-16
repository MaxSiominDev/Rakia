#include "check.h"
#include "engine/timestep.h"
#include "game/weapons.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static Scene scene;
static World world;
static Hangar hangar;
static Targets targets;
static Effects effects;
static Particles particles;
static Weapons weapons;
static Aircraft aircraft;
static Mesh shell;
static MeshGroup group;

// clear of any terrain the noise could plausibly raise, so ground contact only fires when the test means it to
#define SAFE_ALTITUDE 2000.0f

static void setup(void)
{
    int i;

    memset(&scene, 0, sizeof scene);
    memset(&world, 0, sizeof world);
    memset(&targets, 0, sizeof targets);
    memset(&effects, 0, sizeof effects);
    memset(&particles, 0, sizeof particles);
    memset(&weapons, 0, sizeof weapons);
    memset(&shell, 0, sizeof shell);
    memset(&group, 0, sizeof group);
    group.index_count = 3;
    shell.groups = &group;
    shell.group_count = 1;

    effects.particles = &particles;
    for (i = 0; i < EFFECTS_MARKS; i++) {
        effects.marks[i] = scene_add(&scene, &shell, NULL);
    }

    hangar_init(&hangar);
    // distinct outboard distances, so which pylon empties first is never a tie
    world.pylons[0] = v3(2.0f, 0.0f, -1.0f);
    world.pylons[1] = v3(-3.0f, 0.0f, -1.0f);
    world.pylons[2] = v3(4.0f, 0.0f, -1.0f);
    world.pylons[3] = v3(-5.0f, 0.0f, -1.0f);
    world.jet = scene_add(&scene, &shell, NULL);
    world.jet->position = v3(0.0f, SAFE_ALTITUDE, 0.0f);
    world.jet->orientation = quat_identity();
    for (i = 0; i < WORLD_PYLONS; i++) {
        world.props[world.prop_count].entity = scene_add(&scene, &shell, NULL);
        world.props[world.prop_count].kind = PROP_MISSILE;
        hangar.mounted[i] = world.prop_count;
        world.prop_count++;
    }

    aircraft_place(&aircraft, v3(0.0f, SAFE_ALTITUDE, 0.0f), 0.0f, 250.0f, 0.7f);
    weapons_reset(&weapons);
}

static void alive_target(int index, Vec3 position)
{
    targets.list[index].position = position;
    targets.list[index].yaw = 0.0f;
    targets.list[index].min = v3(-5.0f, -5.0f, -5.0f);
    targets.list[index].max = v3(5.0f, 5.0f, 5.0f);
    targets.list[index].alive = 1;
}

static void test_guided_hit(void)
{
    int locked;
    int step;

    setup();
    alive_target(0, v3(0.0f, SAFE_ALTITUDE, 2000.0f));

    locked = weapons_locked(&targets, &aircraft, 0);
    check(locked == 0, "a target dead ahead and within range is locked");
    check(weapons_launch(&weapons, &hangar, &world, &aircraft, locked) == 1, "the launch takes an armed pylon");
    check(weapons.launched == 1, "and is counted as a launch");

    for (step = 0; step < (int)(WEAPONS_LIFETIME * TIMESTEP_HZ) && weapons.hits == 0; step++) {
        weapons_step(&weapons, &targets, &effects, TIMESTEP_DT);
    }
    check(weapons.hits == 1, "the guided missile reaches the target inside its lifetime");
    check(!targets.list[0].alive, "and the target is destroyed");
}

static void test_unguided_expires(void)
{
    Vec3 heading;
    int step;

    setup();
    check(weapons_launch(&weapons, &hangar, &world, &aircraft, -1) == 1, "a launch with no lock still fires");
    check(weapons.shots[0].target < 0, "and carries no target to chase");
    heading = v3_normalize(weapons.shots[0].velocity);

    for (step = 0; step < (int)(WEAPONS_LIFETIME * TIMESTEP_HZ) - 6 && weapons.shots[0].flying; step++) {
        weapons_step(&weapons, &targets, &effects, TIMESTEP_DT);
        check_close(v3_dot(heading, v3_normalize(weapons.shots[0].velocity)), 1.0f, 1e-4f,
                    "an unguided missile keeps the heading it launched with");
    }
    check(weapons.shots[0].flying, "it is still flying just short of its lifetime");

    for (; step < (int)(WEAPONS_LIFETIME * TIMESTEP_HZ) + 6; step++) {
        weapons_step(&weapons, &targets, &effects, TIMESTEP_DT);
    }
    check(!weapons.shots[0].flying, "and self-destructs once its lifetime runs out");
    check(weapons.hits == 0, "without ever counting as a hit");
}

static void test_turn_cap(void)
{
    Vec3 initial_heading;
    float turned_degrees;
    int step;

    setup();
    // forty-five degrees off the nose, inside the seeker's gimbal limit but far enough that an uncapped
    // seeker would snap straight onto it
    alive_target(0, v3(2000.0f, SAFE_ALTITUDE, 2000.0f));
    weapons_launch(&weapons, &hangar, &world, &aircraft, 0);
    initial_heading = v3_normalize(weapons.shots[0].velocity);

    for (step = 0; step < 12; step++) {
        weapons_step(&weapons, &targets, &effects, TIMESTEP_DT);
    }
    turned_degrees =
        acosf(clamped(v3_dot(initial_heading, v3_normalize(weapons.shots[0].velocity)), -1.0f, 1.0f)) / VEC_DEGREES;
    check(turned_degrees > 0.0f, "the seeker does turn toward a target off to the side");
    check(turned_degrees < 8.0f, "but only by a bounded rate, not straight onto it");
}

static void test_launch_outermost(void)
{
    setup();
    check(hangar_missiles(&hangar) == WORLD_PYLONS, "every pylon starts loaded");
    check(weapons_launch(&weapons, &hangar, &world, &aircraft, -1) == 1, "the launch takes the outermost pylon");
    check(hangar.mounted[3] == -1, "which is the one it empties");
    check(hangar_missiles(&hangar) == WORLD_PYLONS - 1, "so the missiles-left count drops by one");

    weapons_launch(&weapons, &hangar, &world, &aircraft, -1);
    check(hangar.mounted[2] == -1, "the next launch takes the next outermost one");
    check(hangar_missiles(&hangar) == WORLD_PYLONS - 2, "and the count follows again");
}

static void test_launch_empty(void)
{
    int i;

    setup();
    for (i = 0; i < WORLD_PYLONS; i++) {
        hangar.mounted[i] = -1;
    }
    check(weapons_launch(&weapons, &hangar, &world, &aircraft, -1) == 0, "an empty jet has nothing left to fire");
    check(weapons.launched == 0, "so nothing is counted as a launch");
}

static void test_step_hit(void)
{
    setup();
    alive_target(0, v3(0.0f, SAFE_ALTITUDE, 100.0f));

    weapons.shots[0].entity = world.props[0].entity;
    weapons.shots[0].position = v3(0.0f, SAFE_ALTITUDE, 97.0f);
    weapons.shots[0].velocity = v3(0.0f, 0.0f, 300.0f);
    weapons.shots[0].target = -1;
    weapons.shots[0].flying = 1;

    weapons_step(&weapons, &targets, &effects, TIMESTEP_DT);
    check(!targets.list[0].alive, "a missile inside the target's box hits it even without guidance");
    check(weapons.hits == 1, "and the hit is counted");
    check(!weapons.shots[0].flying, "the missile itself is spent");
}

static void test_reset_flying(void)
{
    setup();
    weapons_launch(&weapons, &hangar, &world, &aircraft, -1);
    check(weapons.shots[0].flying, "the shot is still in the air");

    weapons_reset(&weapons);
    check(!weapons.shots[0].flying, "reset abandons it");
    check(weapons.shots[0].entity->hidden, "and hides it, or it hangs in the world with nothing to fly it again");
}

static void test_flying_count(void)
{
    setup();
    check(weapons_flying(&weapons) == 0, "nothing is airborne before a launch");

    weapons_launch(&weapons, &hangar, &world, &aircraft, -1);
    weapons_launch(&weapons, &hangar, &world, &aircraft, -1);
    check(weapons_flying(&weapons) == 2, "both launches are in the air");

    weapons.shots[0].time = WEAPONS_LIFETIME;
    weapons_step(&weapons, &targets, &effects, TIMESTEP_DT);
    check(weapons_flying(&weapons) == 1, "one self-destructing drops the count");

    weapons_reset(&weapons);
    check(weapons_flying(&weapons) == 0, "and reset abandons what is left");
}

void test_weapons_main(void)
{
    test_guided_hit();
    test_unguided_expires();
    test_turn_cap();
    test_launch_outermost();
    test_launch_empty();
    test_step_hit();
    test_reset_flying();
    test_flying_count();
}
