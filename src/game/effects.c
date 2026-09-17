#include "game/effects.h"

#include "engine/mesh_data.h"
#include "engine/noise.h"

#include <math.h>
#include <stdio.h>

#define FLASH_FRAME(n) "raw/effects/kenney_smoke_particles/flash/flash0" n ".png"
#define FIRE_FRAME(n) "raw/effects/kenney_smoke_particles/explosion/explosion0" n ".png"
#define SMOKE_FRAME(n) "raw/effects/kenney_smoke_particles/white_puff/whitePuff0" n ".png"
#define FLAME_FRAME(n) "raw/effects/kenney_particle_pack/flame_0" n ".png"
#define SCORCH_TEXTURE "raw/effects/kenney_particle_pack/scorch_02.png"

// every explosion sequence is nine frames, which fits a three by three sheet
#define SHEET_FRAMES 9
#define SHEET_COLUMNS 3
// the flame sheet is four frames, laid out two by two
#define FLAME_FRAMES 4
#define FLAME_COLUMNS 2

#define FLASH_SECONDS 0.3f
#define FIRE_SECONDS 1.6f
#define FIRE_SPRITES 7
#define BURNING_SECONDS 18.0f
#define PUFF_SECONDS 0.35f
#define PUFF_LIFETIME 13.0f
#define PUFF_RISE 4.5f
#define PUFF_DRIFT 1.2f
#define PUFF_GROWTH 1.6f
// the mark floats this far off the ground, which is under the depth buffer's reach a few kilometres out
#define MARK_LIFT 0.4f
// slow enough that overlapping additive sprites do not bleach the plume to white
#define AFTERBURNER_SECONDS 0.04f
#define AFTERBURNER_CORE_OFFSET 0.25f
#define AFTERBURNER_CORE_SIZE 1.1f
#define AFTERBURNER_CORE_LIFE 0.09f
// spread along the plume on every emission, so its whole length is always covered
#define AFTERBURNER_PLUME_START 1.5f
#define AFTERBURNER_PLUME_SEGMENTS 4
#define AFTERBURNER_PLUME_SIZE_NEAR 1.8f
#define AFTERBURNER_PLUME_SIZE_FAR 2.8f
#define AFTERBURNER_PLUME_LIFE_MIN 0.16f
#define AFTERBURNER_PLUME_LIFE_MAX 0.26f
// metres behind the nozzle, from the low speed to the high one
#define AFTERBURNER_LENGTH_MIN 4.5f
#define AFTERBURNER_LENGTH_MAX 6.5f
#define AFTERBURNER_LENGTH_SPEED_LOW 250.0f
#define AFTERBURNER_LENGTH_SPEED_HIGH 400.0f
// metres a second, independent of airspeed
#define AFTERBURNER_PLUME_TURBULENCE 1.2f
#define EXHAUST_SIZE 0.5f
#define EXHAUST_LIFE 0.10f
#define EXHAUST_DRIFT 3.0f
#define TRAIL_SIZE 1.5f
#define TRAIL_LIFETIME 3.0f
#define TRAIL_RISE 1.0f
#define TRAIL_DRIFT 0.5f
#define TRAIL_GROWTH 0.5f

static const Vec3 flash_color = {6.0f, 4.4f, 2.6f};
static const Vec3 flash_fade = {2.4f, 1.2f, 0.4f};
static const Vec3 fire_color = {1.7f, 1.1f, 0.7f};
static const Vec3 fire_fade = {0.22f, 0.20f, 0.19f};
static const Vec3 smoke_color = {0.26f, 0.25f, 0.24f};
static const Vec3 smoke_fade = {0.50f, 0.48f, 0.46f};
static const Vec3 burn_color = {0.10f, 0.09f, 0.08f};
// the flame sheet is a faint near-white haze, so the tint carries the color and runs bright; the plume
// tints keep their other channels near zero, or the ACES curve turns them white
static const Vec3 afterburner_core_color = {36.0f, 31.0f, 22.0f};
static const Vec3 afterburner_core_fade = {18.0f, 13.0f, 7.0f};
static const Vec3 afterburner_plume_inner_color = {14.0f, 2.2f, 0.1f};
static const Vec3 afterburner_plume_inner_fade = {6.0f, 0.9f, 0.05f};
static const Vec3 afterburner_plume_outer_color = {2.2f, 0.3f, 14.0f};
static const Vec3 afterburner_plume_outer_fade = {0.9f, 0.15f, 6.0f};
static const Vec3 exhaust_color = {30.0f, 20.0f, 9.0f};
static const Vec3 exhaust_fade = {13.0f, 5.4f, 1.8f};

static const char *const flash_frames[SHEET_FRAMES] = {
    FLASH_FRAME("0"), FLASH_FRAME("1"), FLASH_FRAME("2"), FLASH_FRAME("3"), FLASH_FRAME("4"),
    FLASH_FRAME("5"), FLASH_FRAME("6"), FLASH_FRAME("7"), FLASH_FRAME("8")
};
static const char *const fire_frames[SHEET_FRAMES] = {
    FIRE_FRAME("0"), FIRE_FRAME("1"), FIRE_FRAME("2"), FIRE_FRAME("3"), FIRE_FRAME("4"),
    FIRE_FRAME("5"), FIRE_FRAME("6"), FIRE_FRAME("7"), FIRE_FRAME("8")
};
static const char *const smoke_frames[SHEET_FRAMES] = {
    SMOKE_FRAME("0"), SMOKE_FRAME("1"), SMOKE_FRAME("2"), SMOKE_FRAME("3"), SMOKE_FRAME("4"),
    SMOKE_FRAME("5"), SMOKE_FRAME("6"), SMOKE_FRAME("7"), SMOKE_FRAME("8")
};
static const char *const flame_frames[FLAME_FRAMES] = {
    FLAME_FRAME("1"), FLAME_FRAME("2"), FLAME_FRAME("3"), FLAME_FRAME("4")
};

static int load_marks(Effects *effects, Scene *scene)
{
    MeshData data;
    int i;

    material_default(&effects->burn);
    effects->burn.kd = burn_color;
    effects->burn.diffuse_map = material_texture(SCORCH_TEXTURE, TEXTURE_CUTOUT);
    if (effects->burn.diffuse_map == 0) {
        return -1;
    }
    if (mesh_data_quad(&data, 1.0f, 1.0f, 1.0f) != 0 || mesh_create(&effects->quad, &data) != 0) {
        fprintf(stderr, "out of memory building the burn marks\n");
        return -1;
    }
    mesh_data_free(&data);

    for (i = 0; i < EFFECTS_MARKS; i++) {
        effects->marks[i] = scene_add(scene, &effects->quad, &effects->burn);
        if (effects->marks[i] == NULL) {
            fprintf(stderr, "the scene has no room for the burn marks\n");
            return -1;
        }
        effects->marks[i]->group_flags[0] = GROUP_BLENDED;
        effects->marks[i]->casts_shadow = 0;
        effects->marks[i]->hidden = 1;
    }

    return 0;
}

int effects_init(Effects *effects, Particles *particles, Scene *scene)
{
    effects->particles = particles;
    effects->flash_sheet = particles_sheet(particles, flash_frames, SHEET_FRAMES, SHEET_COLUMNS, TEXTURE_CUTOUT);
    effects->fire_sheet = particles_sheet(particles, fire_frames, SHEET_FRAMES, SHEET_COLUMNS, TEXTURE_CUTOUT);
    // smoke drifts far and shrinks on screen, so it keeps its mipmaps
    effects->smoke_sheet = particles_sheet(particles, smoke_frames, SHEET_FRAMES, SHEET_COLUMNS, TEXTURE_CUTOUT);
    // under a third opaque, so mipmaps would smear it away; it never lives long enough to shrink
    effects->flame_sheet = particles_sheet(particles, flame_frames, FLAME_FRAMES, FLAME_COLUMNS, TEXTURE_SPRITE);
    if (effects->flash_sheet < 0 || effects->fire_sheet < 0 || effects->smoke_sheet < 0 ||
        effects->flame_sheet < 0) {
        return -1;
    }

    return load_marks(effects, scene);
}

void effects_reset(Effects *effects, unsigned int seed)
{
    int i;

    particles_clear(effects->particles);
    effects->random = seed;
    effects->next_mark = 0;
    effects->afterburner_timer = 0.0f;
    for (i = 0; i < EFFECTS_COLUMNS; i++) {
        effects->columns[i].burning = 0.0f;
    }
    for (i = 0; i < EFFECTS_MARKS; i++) {
        effects->marks[i]->hidden = 1;
    }
}

static float spread(Effects *effects, float amount)
{
    return (noise_random(&effects->random) * 2.0f - 1.0f) * amount;
}

static Particle *sprite(Effects *effects, int sheet, Vec3 position, float size, float lifetime)
{
    Particle *particle = particles_spawn(effects->particles);

    particle->position = position;
    particle->size = size;
    particle->lifetime = lifetime;
    particle->rotation = spread(effects, VEC_PI);
    particle->alpha = 1.0f;
    particle->sheet = sheet;
    particle->blend = PARTICLE_ALPHA;
    // runs the whole sequence once over the life by default; callers can override to hold one frame
    particle->frame_rate = (float)(SHEET_FRAMES - 1) / lifetime;

    return particle;
}

static void flash(Effects *effects, Vec3 position, float size)
{
    Particle *particle = sprite(effects, effects->flash_sheet, position, size * 2.0f, FLASH_SECONDS);

    particle->color = flash_color;
    particle->fade_color = flash_fade;
    particle->growth = size;
    particle->blend = PARTICLE_ADDITIVE;
}

static void fireball(Effects *effects, Vec3 position, float size)
{
    int i;

    for (i = 0; i < FIRE_SPRITES; i++) {
        const Vec3 spot = v3(position.x + spread(effects, size * 0.3f), position.y + spread(effects, size * 0.2f),
                             position.z + spread(effects, size * 0.3f));
        const float life = FIRE_SECONDS * (0.7f + noise_random(&effects->random) * 0.6f);
        Particle *particle = sprite(effects, effects->fire_sheet, spot, size * 0.8f, life);

        particle->color = fire_color;
        particle->fade_color = fire_fade;
        particle->growth = size * 0.5f;
        particle->velocity = v3(spread(effects, 2.0f), 2.0f + noise_random(&effects->random) * 3.0f,
                                spread(effects, 2.0f));
        particle->spin = spread(effects, 0.5f);
    }
}

static void puff(Effects *effects, const SmokeColumn *column)
{
    const Vec3 spot = v3(column->position.x + spread(effects, column->size * 0.25f),
                         column->position.y + column->size * 0.3f,
                         column->position.z + spread(effects, column->size * 0.25f));
    Particle *particle = sprite(effects, effects->smoke_sheet, spot, column->size * 0.6f, PUFF_LIFETIME);

    particle->color = smoke_color;
    particle->fade_color = smoke_fade;
    particle->alpha = 0.85f;
    particle->growth = PUFF_GROWTH;
    particle->velocity = v3(spread(effects, PUFF_DRIFT), PUFF_RISE * (0.7f + noise_random(&effects->random) * 0.6f),
                            spread(effects, PUFF_DRIFT));
    particle->spin = spread(effects, 0.15f);
    particle->frame = (int)(noise_random(&effects->random) * (float)SHEET_FRAMES) % SHEET_FRAMES;
    particle->frame_rate = 0.0f;
}

static SmokeColumn *free_column(Effects *effects)
{
    SmokeColumn *oldest = &effects->columns[0];
    int i;

    for (i = 0; i < EFFECTS_COLUMNS; i++) {
        if (effects->columns[i].burning < oldest->burning) {
            oldest = &effects->columns[i];
        }
    }

    return oldest;
}

void effects_explosion(Effects *effects, Vec3 position, float size)
{
    SmokeColumn *column = free_column(effects);

    flash(effects, position, size);
    fireball(effects, position, size);
    column->position = position;
    column->size = size;
    column->burning = BURNING_SECONDS;
    column->next_puff = 0.0f;
}

void effects_burst(Effects *effects, Vec3 position, float size)
{
    Particle *particle;

    flash(effects, position, size);
    particle = sprite(effects, effects->fire_sheet, position, size * 0.9f, FIRE_SECONDS * 0.7f);
    particle->color = fire_color;
    particle->fade_color = fire_fade;
    particle->growth = size * 0.6f;
}

void effects_mark(Effects *effects, Vec3 position, Vec3 normal, float size)
{
    Entity *mark = effects->marks[effects->next_mark];
    const Vec3 up = v3(0.0f, 1.0f, 0.0f);
    const Vec3 axis = v3_cross(up, normal);
    const float sine = v3_length(axis);
    const Quat turn = quat_from_axis_angle(up, spread(effects, VEC_PI));
    // the slopes a target stands on are gentle, so the tilt off level is the arc of that sine
    const Quat tilt = sine > 1e-4f ? quat_from_axis_angle(axis, asinf(clamped(sine, 0.0f, 1.0f))) : quat_identity();

    effects->next_mark = (effects->next_mark + 1) % EFFECTS_MARKS;
    mark->position = v3_add(position, v3_scale(normal, MARK_LIFT));
    mark->orientation = quat_multiply(tilt, turn);
    mark->scale = size;
    mark->hidden = 0;
}

void effects_afterburner(Effects *effects, Vec3 position, Vec3 direction, float speed, int active, float dt)
{
    Vec3 core_base;
    Vec3 core_spot;
    // moving with the jet keeps each sprite at its place behind the nozzle
    Vec3 ride;
    float length;
    Particle *core;
    int i;

    if (!active) {
        effects->afterburner_timer = 0.0f;
        return;
    }
    effects->afterburner_timer -= dt;
    if (effects->afterburner_timer > 0.0f) {
        return;
    }
    effects->afterburner_timer = AFTERBURNER_SECONDS;

    ride = v3_scale(direction, -speed);
    length = AFTERBURNER_LENGTH_MIN + (AFTERBURNER_LENGTH_MAX - AFTERBURNER_LENGTH_MIN) *
             clamped((speed - AFTERBURNER_LENGTH_SPEED_LOW) /
                     (AFTERBURNER_LENGTH_SPEED_HIGH - AFTERBURNER_LENGTH_SPEED_LOW), 0.0f, 1.0f);

    core_base = v3_add(position, v3_scale(direction, AFTERBURNER_CORE_OFFSET));
    core_spot = v3(core_base.x + spread(effects, 0.10f), core_base.y + spread(effects, 0.10f),
                   core_base.z + spread(effects, 0.10f));
    core = sprite(effects, effects->flame_sheet, core_spot, AFTERBURNER_CORE_SIZE + spread(effects, 0.12f),
                  AFTERBURNER_CORE_LIFE + spread(effects, 0.015f));
    core->color = afterburner_core_color;
    core->fade_color = afterburner_core_fade;
    core->blend = PARTICLE_ADDITIVE;
    core->velocity = ride;
    core->growth = AFTERBURNER_CORE_SIZE * 0.5f;
    core->frame = (int)(noise_random(&effects->random) * (float)FLAME_FRAMES) % FLAME_FRAMES;
    core->frame_rate = 0.0f;

    for (i = 0; i < AFTERBURNER_PLUME_SEGMENTS; i++) {
        const float t = ((float)i + noise_random(&effects->random)) / (float)AFTERBURNER_PLUME_SEGMENTS;
        const float along = AFTERBURNER_PLUME_START + t * (length - AFTERBURNER_PLUME_START);
        const Vec3 base = v3_add(position, v3_scale(direction, along));
        const Vec3 spot = v3(base.x + spread(effects, 0.3f), base.y + spread(effects, 0.3f),
                             base.z + spread(effects, 0.3f));
        const Vec3 velocity = v3(ride.x + spread(effects, AFTERBURNER_PLUME_TURBULENCE),
                                 ride.y + spread(effects, AFTERBURNER_PLUME_TURBULENCE),
                                 ride.z + spread(effects, AFTERBURNER_PLUME_TURBULENCE));
        const float size = AFTERBURNER_PLUME_SIZE_NEAR +
                           t * (AFTERBURNER_PLUME_SIZE_FAR - AFTERBURNER_PLUME_SIZE_NEAR);
        const float life = AFTERBURNER_PLUME_LIFE_MIN + t * (AFTERBURNER_PLUME_LIFE_MAX - AFTERBURNER_PLUME_LIFE_MIN);
        // eased, so the orange holds and the violet shows only near the tail
        const float hue = t * t;
        Particle *plume = sprite(effects, effects->flame_sheet, spot, size + spread(effects, 0.3f),
                                 life + spread(effects, 0.03f));

        plume->color = v3_lerp(afterburner_plume_inner_color, afterburner_plume_outer_color, hue);
        plume->fade_color = v3_lerp(afterburner_plume_inner_fade, afterburner_plume_outer_fade, hue);
        plume->blend = PARTICLE_ADDITIVE;
        plume->velocity = velocity;
        plume->growth = AFTERBURNER_PLUME_SIZE_NEAR * 0.35f;
        plume->spin = spread(effects, 0.6f);
        plume->frame = (int)(noise_random(&effects->random) * (float)FLAME_FRAMES) % FLAME_FRAMES;
        plume->frame_rate = 0.0f;
    }
}

void effects_exhaust(Effects *effects, Vec3 position, Vec3 direction, float dt)
{
    const Vec3 spot = v3(position.x + spread(effects, 0.10f), position.y + spread(effects, 0.10f),
                         position.z + spread(effects, 0.10f));
    Particle *particle = sprite(effects, effects->flame_sheet, spot, EXHAUST_SIZE + spread(effects, 0.08f),
                                EXHAUST_LIFE);

    particle->color = exhaust_color;
    particle->fade_color = exhaust_fade;
    particle->blend = PARTICLE_ADDITIVE;
    particle->velocity = v3_scale(direction, EXHAUST_DRIFT);
    particle->position = v3_add(particle->position, v3_scale(particle->velocity, dt));
    particle->frame = (int)(noise_random(&effects->random) * (float)FLAME_FRAMES) % FLAME_FRAMES;
    particle->frame_rate = 0.0f;
}

void effects_trail_puff(Effects *effects, Vec3 position)
{
    const Vec3 spot = v3(position.x + spread(effects, TRAIL_SIZE * 0.25f),
                         position.y + spread(effects, TRAIL_SIZE * 0.25f),
                         position.z + spread(effects, TRAIL_SIZE * 0.25f));
    Particle *particle = sprite(effects, effects->smoke_sheet, spot, TRAIL_SIZE, TRAIL_LIFETIME);

    particle->color = smoke_color;
    particle->fade_color = smoke_fade;
    particle->alpha = 0.85f;
    particle->growth = TRAIL_GROWTH;
    particle->velocity = v3(spread(effects, TRAIL_DRIFT), TRAIL_RISE * (0.7f + noise_random(&effects->random) * 0.6f),
                            spread(effects, TRAIL_DRIFT));
    particle->spin = spread(effects, 0.15f);
    particle->frame = (int)(noise_random(&effects->random) * (float)SHEET_FRAMES) % SHEET_FRAMES;
    particle->frame_rate = 0.0f;
}

void effects_step(Effects *effects, float dt)
{
    int i;

    for (i = 0; i < EFFECTS_COLUMNS; i++) {
        SmokeColumn *column = &effects->columns[i];

        if (column->burning <= 0.0f) {
            continue;
        }
        column->burning -= dt;
        column->next_puff -= dt;
        if (column->next_puff <= 0.0f) {
            puff(effects, column);
            column->next_puff = PUFF_SECONDS;
        }
    }
}
