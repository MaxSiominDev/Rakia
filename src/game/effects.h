#ifndef EFFECTS_H
#define EFFECTS_H

#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/particles.h"
#include "engine/scene.h"
#include "engine/vecmath.h"

// wrecks that can smoke at once, and burn marks kept before the oldest is reused
#define EFFECTS_COLUMNS 6
#define EFFECTS_MARKS 8

typedef struct {
    Vec3 position;
    float size;
    // seconds of smoke left in the wreck, and seconds to the next puff off it
    float burning;
    float next_puff;
} SmokeColumn;

typedef struct {
    Particles *particles;
    // the quad and the scorch material every burn mark uses
    Mesh quad;
    Material burn;
    Entity *marks[EFFECTS_MARKS];
    int next_mark;
    SmokeColumn columns[EFFECTS_COLUMNS];
    int flash_sheet;
    int fire_sheet;
    int smoke_sheet;
    // reseeded every mission, so sprite spread and frame picks come out the same each time
    unsigned int random;
} Effects;

int effects_init(Effects *effects, Particles *particles, Scene *scene);
void effects_reset(Effects *effects, unsigned int seed);
void effects_step(Effects *effects, float dt);
// a flash and a fireball about that size, then a smoke column
void effects_explosion(Effects *effects, Vec3 position, float size);
// flash and fire only, no smoke column, for a missile that times out without a hit
void effects_burst(Effects *effects, Vec3 position, float size);
// the scorched ground under a wreck or an impact, lying along the slope it is burnt into
void effects_mark(Effects *effects, Vec3 position, Vec3 normal, float size);

#endif
