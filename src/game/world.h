#ifndef WORLD_H
#define WORLD_H

#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/scene.h"
#include "engine/vecmath.h"
#include "game/aircraft.h"

#define WORLD_PYLONS 4
#define WORLD_MAX_PROPS 16
// the airbase and the targets share these pools
#define WORLD_MAX_MESHES 24
#define WORLD_MAX_MATERIALS 56

// the airbase in metres: the runway runs along +z with its centerline at x = 0, the apron lies west of its
// southern half, and the jet parks there facing north
#define WORLD_RUNWAY_LENGTH 2500.0f
#define WORLD_RUNWAY_WIDTH 45.0f
#define WORLD_RUNWAY_X 0.0f
#define WORLD_RUNWAY_HEADING 0.0f
#define WORLD_APRON_X 150.0f
#define WORLD_PARK_Z (-1180.0f)
#define WORLD_PARK_HEADING 0.0f
// where the taxiway leaves the apron and where it meets the runway edge
#define WORLD_TAXI_Z (-1120.0f)
#define WORLD_TAXI_CORNER_X 30.0f
// the paving rests just clear of the flattened terrain it would otherwise fight for depth, and the jet stands
// a landing gear leg above it
#define WORLD_PAVING 0.15f
#define WORLD_GEAR_DROP 0.8f
#define WORLD_STAND (WORLD_PAVING + WORLD_GEAR_DROP)

typedef enum {
    PROP_FIXED,
    PROP_DRAGGABLE,
    // the jet and the cart carry missiles inside their own boxes, which the cursor and the push-apart allow for
    PROP_CARRIER,
    PROP_MISSILE
} PropKind;

// something standing on the apron: the cursor picks it, a drag moves it over the ground, and the props it runs
// into push it back out
typedef struct {
    Entity *entity;
    // the model's box in metres around the entity's own origin
    Vec3 min;
    Vec3 max;
    float yaw;
    // the height its origin sits at when it stands on the apron
    float rest;
    // where a missile prop restocks to; meaningless for every other kind
    Vec3 home;
    PropKind kind;
} Prop;

typedef struct {
    Mesh meshes[WORLD_MAX_MESHES];
    Material materials[WORLD_MAX_MATERIALS];
    int mesh_count;
    int material_count;
    // the missile the cart carries and the pylons take
    const Mesh *missile_mesh;
    const Material *missile_materials;
    Entity *jet;
    Entity *gear;
    Entity *roundels;
    // the two air-to-air missiles bolted to the wingtip rails for looks, and where they ride
    Entity *rails[2];
    Vec3 rail_offsets[2];
    Prop props[WORLD_MAX_PROPS];
    int prop_count;
    // where a mounted missile's center hangs, in the jet's own frame, metres
    Vec3 pylons[WORLD_PYLONS];
    // in the jet's own frame, scaled to metres: the box the ground contact walks, the point a camera orbits
    // and the pilot's eye
    Vec3 jet_min;
    Vec3 jet_max;
    Vec3 jet_center;
    Vec3 cockpit_eye;
    // the canopy swings about this point, in the model's own units
    Vec3 canopy_pivot;
} World;

int world_init(World *world, Scene *scene, Aircraft *aircraft);
// one model into the world's pools; the parsed data goes as soon as the mesh is uploaded
int world_load(World *world, const char *relative, Mesh **mesh, Material **materials);
// the jet back on its spot, standing still
void world_park(World *world, Aircraft *aircraft);
// 0 leaves the canopy wide open, 1 shuts it
void world_canopy(World *world, float shut);
// where a point given in the jet's own frame, in metres, ends up in the world
Vec3 world_on_jet(const World *world, Vec3 offset);
// the parts bolted to the jet follow it: the gear, the roundels and the wingtip missiles
void world_follow(World *world);

#endif
