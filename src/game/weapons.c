#include "game/weapons.h"

#include "engine/terrain.h"

#include <math.h>
#include <stddef.h>

// the motor burns for this long, which is where the 150 m/s it adds comes from
#define BOOST_SECONDS 2.0f
#define BOOST_ACCEL 75.0f
#define TOP_SPEED 500.0f
#define TURN_RATE (60.0f * VEC_DEGREES)
// beyond this angle off the nose, the seeker drops the lock and the missile flies straight
#define SEEKER_LIMIT (80.0f * VEC_DEGREES)
// how close the missile has to pass to count as a hit, on top of the target's own box
#define MISSILE_REACH 2.0f
// fireball sizes: a destroyed target, a missile burning out in the air, and a ground impact
#define TARGET_FIRE_MIN 14.0f
#define TARGET_FIRE_MAX 40.0f
#define BURST_FIRE 7.0f
#define GROUND_FIRE 9.0f
#define MARK_SHARE 0.8f

static float target_size(const Target *target)
{
    return clamped(v3_length(v3_sub(target->max, target->min)), TARGET_FIRE_MIN, TARGET_FIRE_MAX);
}

// the nose follows the flight path, the way the jet's own pose is built
static Quat facing(Vec3 direction)
{
    const Quat turn = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), -atan2f(-direction.x, direction.z));
    const Quat nose = quat_from_axis_angle(v3(1.0f, 0.0f, 0.0f), -asinf(clamped(direction.y, -1.0f, 1.0f)));

    return quat_multiply(turn, nose);
}

void weapons_reset(Weapons *weapons)
{
    int i;

    for (i = 0; i < WEAPONS_SHOTS; i++) {
        // a shot still in the air when the hangar reopens is abandoned, not resolved, so it needs hiding too
        if (weapons->shots[i].flying) {
            weapons->shots[i].entity->hidden = 1;
        }
        weapons->shots[i].flying = 0;
    }
    weapons->launched = 0;
    weapons->hits = 0;
}

int weapons_locked(const Targets *targets, const Aircraft *aircraft, int designated)
{
    const Vec3 nose = quat_rotate(aircraft->orientation, v3(0.0f, 0.0f, 1.0f));
    Vec3 to_target;
    float range;

    if (designated < 0 || !targets->list[designated].alive) {
        return -1;
    }
    to_target = v3_sub(target_center(&targets->list[designated]), aircraft->position);
    range = v3_length(to_target);
    if (range > WEAPONS_RANGE || range <= 0.0f) {
        return -1;
    }

    return v3_dot(nose, v3_scale(to_target, 1.0f / range)) > cosf(WEAPONS_CONE) ? designated : -1;
}

// the loaded pylon furthest from the centerline, -1 when none are loaded
static int loaded_pylon(const Hangar *hangar, const World *world)
{
    float outermost = -1.0f;
    int found = -1;
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        const float outboard = fabsf(world->pylons[i].x);

        if (hangar->mounted[i] >= 0 && outboard > outermost) {
            outermost = outboard;
            found = i;
        }
    }

    return found;
}

static Missile *free_shot(Weapons *weapons)
{
    int i;

    for (i = 0; i < WEAPONS_SHOTS; i++) {
        if (!weapons->shots[i].flying) {
            return &weapons->shots[i];
        }
    }

    return NULL;
}

int weapons_launch(Weapons *weapons, Hangar *hangar, World *world, const Aircraft *aircraft, int target)
{
    const int pylon = loaded_pylon(hangar, world);
    Missile *missile = free_shot(weapons);

    if (pylon < 0 || missile == NULL) {
        return 0;
    }
    missile->entity = world->props[hangar->mounted[pylon]].entity;
    missile->position = world_on_jet(world, world->pylons[pylon]);
    missile->velocity = v3_scale(quat_rotate(aircraft->orientation, v3(0.0f, 0.0f, 1.0f)), aircraft->speed);
    missile->target = target;
    missile->time = 0.0f;
    missile->flying = 1;
    // the pylon stays unmounted until a hangar restock fills it again
    hangar->mounted[pylon] = -1;
    weapons->launched++;

    return 1;
}

static void steer(Missile *missile, const Targets *targets, float dt)
{
    const float speed = v3_length(missile->velocity);
    const Vec3 heading = v3_scale(missile->velocity, 1.0f / speed);
    Vec3 to_target;
    Vec3 axis;
    float side;

    if (missile->target < 0 || !targets->list[missile->target].alive) {
        return;
    }
    to_target = v3_normalize(v3_sub(target_center(&targets->list[missile->target]), missile->position));
    if (v3_dot(heading, to_target) < cosf(SEEKER_LIMIT)) {
        // beyond the gimbal limit, drop the lock instead of turning onto a target behind it
        missile->target = -1;
        return;
    }
    axis = v3_cross(heading, to_target);
    side = v3_length(axis);
    if (side > 1e-6f) {
        const float turn = fminf(asinf(clamped(side, 0.0f, 1.0f)), TURN_RATE * dt);

        missile->velocity =
            v3_scale(quat_rotate(quat_from_axis_angle(v3_scale(axis, 1.0f / side), turn), heading), speed);
    }
}

static float boosted(float speed, float time, float dt)
{
    return time <= BOOST_SECONDS ? fminf(speed + BOOST_ACCEL * dt, TOP_SPEED) : speed;
}

void weapons_step(Weapons *weapons, Targets *targets, Effects *effects, float dt)
{
    int i;

    for (i = 0; i < WEAPONS_SHOTS; i++) {
        Missile *missile = &weapons->shots[i];
        const Vec3 from = missile->position;
        float speed;
        float ground;
        int hit;

        if (!missile->flying) {
            continue;
        }
        missile->time += dt;
        steer(missile, targets, dt);
        speed = boosted(v3_length(missile->velocity), missile->time, dt);
        missile->velocity = v3_scale(v3_normalize(missile->velocity), speed);
        missile->position = v3_add(from, v3_scale(missile->velocity, dt));
        ground = fmaxf(terrain_height(missile->position.x, missile->position.z), TERRAIN_SEA_LEVEL);

        hit = targets_hit(targets, from, missile->position, MISSILE_REACH);
        if (hit >= 0) {
            Target *target = &targets->list[hit];
            const Vec3 point = target_center(target);
            const float size = target_size(target);

            targets_destroy(targets, hit);
            effects_explosion(effects, point, size);
            effects_mark(effects, target->position, terrain_normal(target->position.x, target->position.z),
                         size * MARK_SHARE);
            weapons->hits++;
        } else if (missile->position.y <= ground) {
            const Vec3 point = v3(missile->position.x, ground, missile->position.z);
            const int dry = terrain_height(point.x, point.z) > TERRAIN_SEA_LEVEL;

            effects_explosion(effects, point, GROUND_FIRE);
            if (dry) {
                effects_mark(effects, point, terrain_normal(point.x, point.z), GROUND_FIRE * MARK_SHARE);
            }
        } else if (missile->time >= WEAPONS_LIFETIME) {
            effects_burst(effects, missile->position, BURST_FIRE);
        } else {
            missile->entity->position = missile->position;
            missile->entity->orientation = facing(v3_normalize(missile->velocity));
            continue;
        }
        missile->entity->hidden = 1;
        missile->flying = 0;
    }
}

int weapons_flying(const Weapons *weapons)
{
    int count = 0;
    int i;

    for (i = 0; i < WEAPONS_SHOTS; i++) {
        count += weapons->shots[i].flying;
    }

    return count;
}