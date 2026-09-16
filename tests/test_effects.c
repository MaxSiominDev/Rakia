#include "check.h"
#include "engine/particles.h"
#include "engine/vecmath.h"
#include "game/effects.h"

#include <string.h>

// one core plus AFTERBURNER_PLUME_SEGMENTS from effects.c
#define AFTERBURNER_SPRITES_PER_EMISSION 5

static Particles particles;
static Effects effects;

static void setup(void)
{
    memset(&particles, 0, sizeof particles);
    memset(&effects, 0, sizeof effects);
    effects.particles = &particles;
    effects.flash_sheet = 0;
    effects.fire_sheet = 1;
    effects.smoke_sheet = 2;
    effects.flame_sheet = 3;
    effects.random = 1u;
}

static int live_count(void)
{
    int count = 0;
    int i;

    for (i = 0; i < PARTICLES_MAX; i++) {
        if (particles.pool[i].lifetime > 0.0f) {
            count++;
        }
    }

    return count;
}

static void test_afterburner_idle(void)
{
    const Vec3 position = v3(0.0f, 100.0f, 0.0f);
    const Vec3 direction = v3(0.0f, 0.0f, -1.0f);
    int step;

    setup();
    for (step = 0; step < 30; step++) {
        effects_afterburner(&effects, position, direction, 300.0f, 0, 1.0f / 120.0f);
    }
    check(live_count() == 0, "no sprite spawns while active is false");
}

static void test_afterburner_spawn(void)
{
    const Vec3 position = v3(0.0f, 100.0f, 0.0f);
    const Vec3 direction = v3(0.0f, 0.0f, -1.0f);
    int i;

    setup();
    effects_afterburner(&effects, position, direction, 300.0f, 1, 1.0f / 120.0f);

    check(live_count() == AFTERBURNER_SPRITES_PER_EMISSION,
          "the timer starts at zero, so the first call spawns the whole plume right away");
    for (i = 0; i < AFTERBURNER_SPRITES_PER_EMISSION; i++) {
        check(particles.pool[i].sheet == effects.flame_sheet, "every sprite draws from the flame sheet");
        check(particles.pool[i].blend == PARTICLE_ADDITIVE, "every sprite blends additively");
    }
}

static void test_afterburner_repeats(void)
{
    const Vec3 position = v3(0.0f, 100.0f, 0.0f);
    const Vec3 direction = v3(0.0f, 0.0f, -1.0f);
    int step;

    setup();
    for (step = 0; step < 30; step++) {
        effects_afterburner(&effects, position, direction, 300.0f, 1, 1.0f / 120.0f);
    }
    check(live_count() > AFTERBURNER_SPRITES_PER_EMISSION,
          "further steps keep feeding the pool, not just the first one");
}

static void test_plume_velocity(void)
{
    const Vec3 position = v3(0.0f, 100.0f, 0.0f);
    const Vec3 direction = v3(0.0f, 0.0f, -1.0f);
    const float speed = 300.0f;
    int i;

    setup();
    effects_afterburner(&effects, position, direction, speed, 1, 1.0f / 120.0f);

    for (i = 0; i < PARTICLES_MAX; i++) {
        float along;

        if (particles.pool[i].lifetime <= 0.0f) {
            continue;
        }
        along = -v3_dot(particles.pool[i].velocity, direction);
        check_close(along, speed, 5.0f, "each sprite's velocity matches the jet's, so it is not left behind");
    }
}

static void test_plume_reach(void)
{
    const Vec3 position = v3(0.0f, 100.0f, 0.0f);
    const Vec3 direction = v3(0.0f, 0.0f, -1.0f);
    float slow_reach = 0.0f;
    float fast_reach = 0.0f;
    int i;

    setup();
    effects_afterburner(&effects, position, direction, 250.0f, 1, 1.0f / 120.0f);
    for (i = 0; i < AFTERBURNER_SPRITES_PER_EMISSION; i++) {
        const float along = v3_dot(v3_sub(particles.pool[i].position, position), direction);

        if (along > slow_reach) {
            slow_reach = along;
        }
    }

    setup();
    effects_afterburner(&effects, position, direction, 400.0f, 1, 1.0f / 120.0f);
    for (i = 0; i < AFTERBURNER_SPRITES_PER_EMISSION; i++) {
        const float along = v3_dot(v3_sub(particles.pool[i].position, position), direction);

        if (along > fast_reach) {
            fast_reach = along;
        }
    }

    check(fast_reach > slow_reach, "the plume reaches further behind the nozzle when the jet is faster");
    check(slow_reach > 2.0f && fast_reach < 8.0f, "the plume's reach stays near the requested 4 to 7 m band");
}

static void test_exhaust_spawn(void)
{
    setup();
    effects_exhaust(&effects, v3(0.0f, 50.0f, 0.0f), v3(0.0f, 0.0f, -1.0f), 1.0f / 120.0f);

    check(live_count() == 1, "one exhaust sprite spawns per call");
    check(particles.pool[0].sheet == effects.flame_sheet, "it draws from the flame sheet");
    check(particles.pool[0].blend == PARTICLE_ADDITIVE, "and blends additively");
    check(particles.pool[0].lifetime >= 0.08f && particles.pool[0].lifetime <= 0.12f,
          "its lifetime sits in the short exhaust range");
}

static void test_trail_spawn(void)
{
    setup();
    effects_trail_puff(&effects, v3(0.0f, 50.0f, 0.0f));

    check(live_count() == 1, "one trail sprite spawns per call");
    check(particles.pool[0].sheet == effects.smoke_sheet, "it draws from the smoke sheet");
    check(particles.pool[0].blend == PARTICLE_ALPHA, "and blends like ordinary smoke");
    check(particles.pool[0].lifetime >= 2.5f && particles.pool[0].lifetime <= 4.0f,
          "its lifetime sits in the trail's fade-out range");
}

void test_effects_main(void)
{
    test_afterburner_idle();
    test_afterburner_spawn();
    test_afterburner_repeats();
    test_plume_velocity();
    test_plume_reach();
    test_exhaust_spawn();
    test_trail_spawn();
}
