#include "game/effects.h"

#include "engine/assets.h"
#include "engine/mesh_data.h"
#include "engine/noise.h"

#include <math.h>
#include <stdio.h>

#define FLASH_FRAME(n) "raw/effects/kenney_smoke_particles/flash/flash0" n ".png"
#define FIRE_FRAME(n) "raw/effects/kenney_smoke_particles/explosion/explosion0" n ".png"
#define SMOKE_FRAME(n) "raw/effects/kenney_smoke_particles/white_puff/whitePuff0" n ".png"
#define SCORCH_TEXTURE "raw/effects/kenney_particle_pack/scorch_02.png"

// every sequence is nine frames, which fits a three by three sheet
#define SHEET_FRAMES 9
#define SHEET_COLUMNS 3

#define FLASH_SECONDS 0.3f
#define FIRE_SECONDS 1.1f
#define FIRE_SPRITES 5
#define BURNING_SECONDS 8.0f
#define PUFF_SECONDS 0.35f
#define PUFF_LIFETIME 13.0f
#define PUFF_RISE 4.5f
#define PUFF_DRIFT 1.2f
#define PUFF_GROWTH 1.6f
// the mark floats this far off the ground, which is under the depth buffer's reach a few kilometres out
#define MARK_LIFT 0.4f

static const Vec3 flash_color = {6.0f, 4.4f, 2.6f};
static const Vec3 flash_fade = {2.4f, 1.2f, 0.4f};
static const Vec3 fire_color = {1.7f, 1.1f, 0.7f};
static const Vec3 fire_fade = {0.22f, 0.20f, 0.19f};
static const Vec3 smoke_color = {0.26f, 0.25f, 0.24f};
static const Vec3 smoke_fade = {0.50f, 0.48f, 0.46f};
static const Vec3 burn_color = {0.10f, 0.09f, 0.08f};

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

static int load_sheet(Particles *particles, const char *const *frames)
{
    char paths[SHEET_FRAMES][ASSETS_PATH_MAX];
    const char *list[SHEET_FRAMES];
    int i;

    for (i = 0; i < SHEET_FRAMES; i++) {
        if (assets_path(paths[i], sizeof paths[i], frames[i]) != 0) {
            fprintf(stderr, "asset path is too long: %s\n", frames[i]);
            return -1;
        }
        list[i] = paths[i];
    }

    return particles_sheet(particles, list, SHEET_FRAMES, SHEET_COLUMNS);
}

static int load_marks(Effects *effects, Scene *scene)
{
    char path[ASSETS_PATH_MAX];
    MeshData data;
    int i;

    if (assets_path(path, sizeof path, SCORCH_TEXTURE) != 0) {
        fprintf(stderr, "asset path is too long: %s\n", SCORCH_TEXTURE);
        return -1;
    }
    material_default(&effects->burn);
    effects->burn.kd = burn_color;
    effects->burn.diffuse_map = material_texture(path, TEXTURE_CUTOUT);
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
    effects->flash_sheet = load_sheet(particles, flash_frames);
    effects->fire_sheet = load_sheet(particles, fire_frames);
    effects->smoke_sheet = load_sheet(particles, smoke_frames);
    if (effects->flash_sheet < 0 || effects->fire_sheet < 0 || effects->smoke_sheet < 0) {
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
