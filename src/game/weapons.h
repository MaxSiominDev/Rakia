#ifndef WEAPONS_H
#define WEAPONS_H

#include "engine/scene.h"
#include "engine/vecmath.h"
#include "game/aircraft.h"
#include "game/effects.h"
#include "game/hangar.h"
#include "game/targets.h"
#include "game/world.h"

// one shot for every pylon the hangar filled
#define WEAPONS_SHOTS WORLD_PYLONS
// the cone the seeker looks through, and how far it still sees
#define WEAPONS_CONE (30.0f * VEC_DEGREES)
#define WEAPONS_RANGE 3000.0f
#define WEAPONS_LIFETIME 8.0f

typedef struct {
    // the prop's own entity, moved to draw the missile in flight
    Entity *entity;
    Vec3 position;
    Vec3 velocity;
    // the target it was fired at, -1 when it left the rail without a lock
    int target;
    float time;
    int flying;
} Missile;

typedef struct {
    Missile shots[WEAPONS_SHOTS];
    int launched;
    int hits;
} Weapons;

void weapons_reset(Weapons *weapons);
// the designated target once it lies inside the cone and within reach, -1 while it does not
int weapons_locked(const Targets *targets, const Aircraft *aircraft, int designated);
// 1 when a missile launches off the outermost loaded pylon, 0 when every pylon is empty
int weapons_launch(Weapons *weapons, Hangar *hangar, World *world, const Aircraft *aircraft, int target);
void weapons_step(Weapons *weapons, Targets *targets, Effects *effects, float dt);
// shots still in the air, none of which have hit, missed or expired yet
int weapons_flying(const Weapons *weapons);

#endif
