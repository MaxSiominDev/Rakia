#include "game/targets.h"

#include "engine/noise.h"
#include "engine/terrain.h"

#include <math.h>
#include <stdio.h>

#define RADOME_MODEL "raw/targets/radome_geodesic_dome_quaternius/radome_geodesic_dome.obj"
// the raw model is 8.5 m across, too small for a radar station without this scale
#define RADOME_SCALE 1.5f
#define DISH_MODEL "raw/targets/radar_dish_quaternius/radar_dish.obj"
#define DISH_SCALE 3.0f
#define SAM_MODEL "raw/targets/sam_2k12_kub/sam_2k12_kub.obj"
// the launcher is 3134 units long and the real vehicle 7.4 m
#define SAM_SCALE 0.0024f
#define URAL_MODEL "raw/targets/truck_ural/truck_ural.obj"
#define URAL_SCALE 0.01f
#define M939_MODEL "raw/targets/truck_m939/truck_m939.obj"
#define DEPOT_MODEL "raw/targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks.obj"

// tries per target, spread around the full circle, so a sector that is all sea still finds ground
#define TARGET_TRIES 24
// metres above sea level required, more than the waves reach, for a spot to count as dry
#define TARGET_DRY 4.0f
#define TARGET_REMOVE_SECONDS 0.6f

// every model a target is built from, in target index order; the radar station takes two
static const struct {
    int target;
    const char *model;
    float scale;
    Vec3 offset;
} models[TARGET_MODELS] = {
    {0, RADOME_MODEL, RADOME_SCALE, {0.0f, 0.0f, 0.0f}},
    {0, DISH_MODEL, DISH_SCALE, {10.0f, 0.0f, -3.0f}},
    {1, SAM_MODEL, SAM_SCALE, {0.0f, 0.0f, 0.0f}},
    {2, URAL_MODEL, URAL_SCALE, {0.0f, 0.0f, 0.0f}},
    {3, M939_MODEL, 1.0f, {0.0f, 0.0f, 0.0f}},
    {4, DEPOT_MODEL, 1.0f, {0.0f, 0.0f, 0.0f}}
};

static int on_dry_land(Vec3 spot)
{
    return !terrain_on_base(spot.x, spot.z) && spot.y > TERRAIN_SEA_LEVEL + TARGET_DRY;
}

static const float around[4][2] = {{1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f}};

// four points at the reach distance from the spot approximate the corners the model actually rests on
static int ground_is_level(Vec3 spot)
{
    int i;

    for (i = 0; i < 4; i++) {
        const float x = spot.x + around[i][0] * TARGETS_FLAT_REACH;
        const float z = spot.z + around[i][1] * TARGETS_FLAT_REACH;

        if (fabsf(terrain_height(x, z) - spot.y) > TARGETS_FLAT_DROP) {
            return 0;
        }
    }

    return 1;
}

static int clear_of_others(const TargetSpot *spots, int count, Vec3 spot)
{
    int i;

    for (i = 0; i < count; i++) {
        if (v3_length(v3(spots[i].position.x - spot.x, 0.0f, spots[i].position.z - spot.z)) < TARGETS_SPACING) {
            return 0;
        }
    }

    return 1;
}

void targets_layout(unsigned int seed, TargetSpot *spots)
{
    const float sector = 2.0f * VEC_PI / (float)TARGETS_COUNT;
    unsigned int state = seed;
    int i;

    for (i = 0; i < TARGETS_COUNT; i++) {
        // each target starts in its own sector of the circle, so the five spread around the base, not into one corner
        const float start = ((float)i + noise_random(&state)) * sector;
        int attempt;

        spots[i].yaw = noise_random(&state) * 2.0f * VEC_PI;
        for (attempt = 0; attempt < TARGET_TRIES; attempt++) {
            const float angle = start + (float)attempt * (2.0f * VEC_PI / (float)TARGET_TRIES);
            const float range = TARGETS_NEAR + (TARGETS_FAR - TARGETS_NEAR) * noise_random(&state);
            const float x = range * sinf(angle);
            const float z = range * cosf(angle);

            spots[i].position = v3(x, terrain_height(x, z), z);
            if (on_dry_land(spots[i].position) && ground_is_level(spots[i].position) &&
                clear_of_others(spots, i, spots[i].position)) {
                break;
            }
        }
    }
}

int targets_init(Targets *targets, World *world, Scene *scene)
{
    int i;

    for (i = 0; i < TARGET_MODELS; i++) {
        Target *target = &targets->list[models[i].target];
        Mesh *mesh;
        Material *materials;
        Entity *entity;
        Vec3 min;
        Vec3 max;

        if (world_load(world, models[i].model, &mesh, &materials) != 0) {
            return -1;
        }
        entity = scene_add(scene, mesh, materials);
        if (entity == NULL) {
            fprintf(stderr, "the scene has no room for %s\n", models[i].model);
            return -1;
        }
        entity->scale = models[i].scale;
        min = v3_add(v3_scale(mesh->bounds_min, models[i].scale), models[i].offset);
        max = v3_add(v3_scale(mesh->bounds_max, models[i].scale), models[i].offset);
        if (target->part_count == 0) {
            target->min = min;
            target->max = max;
        } else {
            target->min = v3(fminf(target->min.x, min.x), fminf(target->min.y, min.y), fminf(target->min.z, min.z));
            target->max = v3(fmaxf(target->max.x, max.x), fmaxf(target->max.y, max.y), fmaxf(target->max.z, max.z));
        }
        target->offsets[target->part_count] = models[i].offset;
        target->parts[target->part_count++] = entity;
    }

    return 0;
}

void targets_place(Targets *targets, unsigned int seed)
{
    TargetSpot spots[TARGETS_COUNT];
    int i;

    targets_layout(seed, spots);
    for (i = 0; i < TARGETS_COUNT; i++) {
        Target *target = &targets->list[i];
        int part;

        target->position = spots[i].position;
        target->yaw = spots[i].yaw;
        target->alive = 1;
        target->removing = 0.0f;
        for (part = 0; part < target->part_count; part++) {
            Entity *entity = target->parts[part];
            const Vec3 offset = quat_rotate(quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), target->yaw),
                                            target->offsets[part]);

            // each part samples its own ground height, kept within TARGETS_FLAT_DROP of the rest by the level test
            entity->position = v3_add(target->position, offset);
            entity->position.y = terrain_height(entity->position.x, entity->position.z);
            entity->orientation = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), target->yaw);
            entity->hidden = 0;
            entity->casts_shadow = 1;
        }
    }
}

void targets_step(Targets *targets, float dt)
{
    int i;

    for (i = 0; i < TARGETS_COUNT; i++) {
        Target *target = &targets->list[i];
        int part;

        if (target->removing <= 0.0f) {
            continue;
        }
        target->removing -= dt;
        if (target->removing > 0.0f) {
            continue;
        }
        for (part = 0; part < target->part_count; part++) {
            target->parts[part]->hidden = 1;
            target->parts[part]->casts_shadow = 0;
        }
    }
}

Hitbox target_hitbox(const Target *target)
{
    Hitbox box;

    box.position = target->position;
    box.yaw = target->yaw;
    box.min = target->min;
    box.max = target->max;

    return box;
}

Vec3 target_center(const Target *target)
{
    const Vec3 middle = v3_lerp(target->min, target->max, 0.5f);

    return v3_add(target->position, quat_rotate(quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), target->yaw), middle));
}

int targets_alive(const Targets *targets)
{
    int count = 0;
    int i;

    for (i = 0; i < TARGETS_COUNT; i++) {
        count += targets->list[i].alive;
    }

    return count;
}

int targets_nearest(const Targets *targets, Vec3 from)
{
    float best = 0.0f;
    int found = -1;
    int i;

    for (i = 0; i < TARGETS_COUNT; i++) {
        const float range = v3_length(v3_sub(targets->list[i].position, from));

        if (targets->list[i].alive && (found < 0 || range < best)) {
            best = range;
            found = i;
        }
    }

    return found;
}

int targets_next(const Targets *targets, int current)
{
    int i;

    for (i = 1; i <= TARGETS_COUNT; i++) {
        const int index = (current + i) % TARGETS_COUNT;

        if (targets->list[index].alive) {
            return index;
        }
    }

    return -1;
}

int targets_hit(const Targets *targets, Vec3 from, Vec3 to, float radius)
{
    int i;

    for (i = 0; i < TARGETS_COUNT; i++) {
        Hitbox box;

        if (!targets->list[i].alive) {
            continue;
        }
        box = target_hitbox(&targets->list[i]);
        if (collision_segment(&box, from, to, radius)) {
            return i;
        }
    }

    return -1;
}

void targets_destroy(Targets *targets, int index)
{
    targets->list[index].alive = 0;
    targets->list[index].removing = TARGET_REMOVE_SECONDS;
}
