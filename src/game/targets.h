#ifndef TARGETS_H
#define TARGETS_H

#include "engine/collision.h"
#include "engine/scene.h"
#include "engine/vecmath.h"
#include "game/world.h"

#define TARGETS_COUNT 5
// the radar station is a dome with a dish beside it; every other target is one model
#define TARGET_PARTS 2
#define TARGET_MODELS (TARGETS_COUNT + 1)

// how far from the airbase the targets stand, how far apart they keep and how level the ground under one has
// to be, measured this far out from its middle
#define TARGETS_NEAR 3000.0f
#define TARGETS_FAR 10000.0f
#define TARGETS_SPACING 1000.0f
#define TARGETS_FLAT_REACH 18.0f
#define TARGETS_FLAT_DROP 2.5f

typedef struct {
    Vec3 position;
    float yaw;
} TargetSpot;

typedef struct {
    Entity *parts[TARGET_PARTS];
    // where each part stands in the target's own frame
    Vec3 offsets[TARGET_PARTS];
    int part_count;
    Vec3 position;
    float yaw;
    // the box that takes hits, in metres around the target's own origin
    Vec3 min;
    Vec3 max;
    int alive;
    // seconds left before the wreck's parts are hidden
    float removing;
} Target;

typedef struct {
    Target list[TARGETS_COUNT];
} Targets;

// where the mission's targets stand, from the seed alone
void targets_layout(unsigned int seed, TargetSpot *spots);
int targets_init(Targets *targets, World *world, Scene *scene);
void targets_place(Targets *targets, unsigned int seed);
void targets_step(Targets *targets, float dt);
Hitbox target_hitbox(const Target *target);
// the point guidance and fire aim at: the middle of the target's own box, turned and placed like it
Vec3 target_center(const Target *target);
int targets_alive(const Targets *targets);
// the live target nearest to a point, -1 when none are left
int targets_nearest(const Targets *targets, Vec3 from);
// the live target after this one, wrapping round to the first
int targets_next(const Targets *targets, int current);
// the live target the path from one point to the other runs into, swept with a ball of that radius
int targets_hit(const Targets *targets, Vec3 from, Vec3 to, float radius);
void targets_destroy(Targets *targets, int index);

#endif
