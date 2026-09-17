#include "engine/foliage.h"

#include "engine/gl_ext.h"
#include "engine/material.h"
#include "engine/noise.h"
#include "engine/terrain.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHADOW_UNIT 1
// a plant is drawn out to this many metres per metre of its height
#define FADE_PER_METRE 400.0f
// the far cap on that, and the radius of the ring of cells that gets built
#define FADE_END 2000.0f
// tries per cell; how many of them take root depends on the biome
#define CANDIDATES 3072
#define BUILD_BUDGET 1
#define ALPHA_CUTOFF 0.5f
// nothing grows on a face steeper than about 35 degrees
#define SLOPE_LIMIT 0.82f
// far enough that every card faces the same way, as it would under a parallel light
#define SUN_DISTANCE 100000.0f

#define FOLIAGE_IMAGE(dir, name) "raw/foliage/" dir "/" name ".png"

// billboards keep the aspect of their cutout; the chance columns are for the desert and for the green north
static const struct {
    const char *image;
    float width;
    float height;
    float desert;
    float green;
} kinds[FOLIAGE_KINDS] = {
    {FOLIAGE_IMAGE("desert_shrubs_trigger_rally_onsemeliot", "dry-grass"), 1.8f, 1.8f, 0.70f, 0.30f},
    {FOLIAGE_IMAGE("desert_shrubs_trigger_rally_onsemeliot", "thorn-bush"), 2.4f, 2.4f, 0.45f, 0.20f},
    {FOLIAGE_IMAGE("acacia_standin_trigger_rally_onsemeliot", "wide-tree"), 7.0f, 7.0f, 0.05f, 0.35f},
    {FOLIAGE_IMAGE("conifers_photo_rubberduck", "spruce_1"), 5.5f, 11.0f, 0.0f, 0.60f}
};

typedef struct {
    Vec3 position;
    float u;
    float v;
    float half_width;
    float height;
} FoliageVertex;

static unsigned int plant_hash(int ix, int iz, int index)
{
    unsigned int h = (unsigned int)ix * 0x8DA6B343u + (unsigned int)iz * 0xD8163841u +
                     (unsigned int)index * 0xCB1AB31Fu;

    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;

    return h;
}

int foliage_place(int ix, int iz, FoliagePlant *plants, int max)
{
    const float origin_x = (float)ix * FOLIAGE_CELL;
    const float origin_z = (float)iz * FOLIAGE_CELL;
    int count = 0;
    int i;

    for (i = 0; i < CANDIDATES && count < max; i++) {
        unsigned int state = plant_hash(ix, iz, i);
        const float x = origin_x + FOLIAGE_CELL * noise_random(&state);
        const float z = origin_z + FOLIAGE_CELL * noise_random(&state);
        const int kind = (int)(noise_random(&state) * FOLIAGE_KINDS);
        const float roll = noise_random(&state);
        const float scale = 0.75f + 0.6f * noise_random(&state);
        TerrainPoint point;

        if (terrain_on_base(x, z)) {
            continue;
        }
        point = terrain_sample(x, z);
        if (point.height < TERRAIN_SEA_LEVEL + 1.0f) {
            continue;
        }
        if (roll > kinds[kind].desert + (kinds[kind].green - kinds[kind].desert) * point.green) {
            continue;
        }
        if (terrain_normal(x, z).y < SLOPE_LIMIT) {
            continue;
        }
        plants[count].position = v3(x, point.height, z);
        plants[count].half_width = kinds[kind].width * scale * 0.5f;
        plants[count].height = kinds[kind].height * scale;
        plants[count].kind = kind;
        count++;
    }

    return count;
}

static const float quad_corners[6][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
                                         {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

static int fill_vertices(FoliageVertex *vertices, const FoliagePlant *plants, int count, FoliageChunk *chunk)
{
    int written = 0;
    int kind;

    for (kind = 0; kind < FOLIAGE_KINDS; kind++) {
        int i;

        chunk->first[kind] = written;
        for (i = 0; i < count; i++) {
            int corner;

            if (plants[i].kind != kind) {
                continue;
            }
            for (corner = 0; corner < 6; corner++) {
                FoliageVertex *vertex = &vertices[written++];

                vertex->position = plants[i].position;
                vertex->u = quad_corners[corner][0];
                vertex->v = quad_corners[corner][1];
                vertex->half_width = plants[i].half_width;
                vertex->height = plants[i].height;
            }
        }
        chunk->count[kind] = written - chunk->first[kind];
    }

    return written;
}

static int free_slot(Foliage *foliage)
{
    int oldest = 0;
    int i;

    if (foliage->chunk_count < FOLIAGE_CHUNK_SLOTS) {
        return foliage->chunk_count++;
    }
    for (i = 1; i < foliage->chunk_count; i++) {
        if (foliage->chunks[i].touched < foliage->chunks[oldest].touched) {
            oldest = i;
        }
    }
    glDeleteBuffers(1, &foliage->chunks[oldest].buffer);

    return oldest;
}

static int build_chunk(Foliage *foliage, int ix, int iz)
{
    FoliagePlant *plants = malloc(sizeof *plants * FOLIAGE_PER_CELL);
    FoliageVertex *vertices = malloc(sizeof *vertices * FOLIAGE_PER_CELL * 6);
    FoliageChunk *chunk;
    int planted;
    int slot;
    int written;

    if (plants == NULL || vertices == NULL) {
        free(plants);
        free(vertices);
        return -1;
    }
    planted = foliage_place(ix, iz, plants, FOLIAGE_PER_CELL);
    slot = free_slot(foliage);
    chunk = &foliage->chunks[slot];
    written = fill_vertices(vertices, plants, planted, chunk);
    chunk->ix = ix;
    chunk->iz = iz;

    glGenBuffers(1, &chunk->buffer);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof *vertices * (size_t)written), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free(plants);
    free(vertices);

    return slot;
}

static int find_chunk(const Foliage *foliage, int ix, int iz)
{
    int i;

    for (i = 0; i < foliage->chunk_count; i++) {
        if (foliage->chunks[i].ix == ix && foliage->chunks[i].iz == iz) {
            return i;
        }
    }

    return -1;
}

void foliage_update(Foliage *foliage, Vec3 center)
{
    const int budget = foliage->chunk_count == 0 ? FOLIAGE_CHUNK_SLOTS : BUILD_BUDGET;
    const int first_x = (int)floorf((center.x - FADE_END) / FOLIAGE_CELL);
    const int last_x = (int)floorf((center.x + FADE_END) / FOLIAGE_CELL);
    const int first_z = (int)floorf((center.z - FADE_END) / FOLIAGE_CELL);
    const int last_z = (int)floorf((center.z + FADE_END) / FOLIAGE_CELL);
    int built = 0;
    int ix;
    int iz;

    foliage->frame++;
    foliage->visible_count = 0;
    for (iz = first_z; iz <= last_z; iz++) {
        for (ix = first_x; ix <= last_x; ix++) {
            int slot = find_chunk(foliage, ix, iz);

            if (slot < 0) {
                if (built == budget) {
                    continue;
                }
                slot = build_chunk(foliage, ix, iz);
                if (slot < 0) {
                    continue;
                }
                built++;
            }
            foliage->chunks[slot].touched = foliage->frame;
            foliage->visible[foliage->visible_count++] = slot;
        }
    }
}

static void draw_chunks(const Foliage *foliage)
{
    int i;

    glEnableVertexAttribArray(SHADER_ATTRIBUTE_POSITION);
    glEnableVertexAttribArray(SHADER_ATTRIBUTE_CORNER);
    // the cards have no back, so both sides are drawn and the winding never matters
    glDisable(GL_CULL_FACE);

    for (i = 0; i < foliage->visible_count; i++) {
        const FoliageChunk *chunk = &foliage->chunks[foliage->visible[i]];
        int kind;

        glBindBuffer(GL_ARRAY_BUFFER, chunk->buffer);
        glVertexAttribPointer(SHADER_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(FoliageVertex),
                              (const void *)(uintptr_t)offsetof(FoliageVertex, position));
        glVertexAttribPointer(SHADER_ATTRIBUTE_CORNER, 4, GL_FLOAT, GL_FALSE, sizeof(FoliageVertex),
                              (const void *)(uintptr_t)offsetof(FoliageVertex, u));
        for (kind = 0; kind < FOLIAGE_KINDS; kind++) {
            if (chunk->count[kind] == 0) {
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, foliage->images[kind]);
            glDrawArrays(GL_TRIANGLES, chunk->first[kind], chunk->count[kind]);
        }
    }

    glEnable(GL_CULL_FACE);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_CORNER);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_POSITION);
}

void foliage_draw(Foliage *foliage, Mat4 view_projection, const Camera *camera, const Light *light,
                  const Shadow *shadow)
{
    Shader *shader = &foliage->shader;

    glUseProgram(shader->program);
    shader_set_mat4(shader, "u_view_projection", view_projection);
    shader_set_mat4(shader, "u_shadow_matrix", shadow->texture_matrix);
    shader_set_vec3(shader, "u_view_origin", camera->eye);
    shader_set_vec3(shader, "u_camera_position", camera->eye);
    light_apply(light, shader);
    glActiveTexture(GL_TEXTURE0 + SHADOW_UNIT);
    glBindTexture(GL_TEXTURE_2D, shadow->texture);
    glActiveTexture(GL_TEXTURE0);

    draw_chunks(foliage);

    glActiveTexture(GL_TEXTURE0 + SHADOW_UNIT);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

void foliage_draw_depth(Foliage *foliage, Mat4 view_projection, Vec3 camera_position, Vec3 sun_direction)
{
    Shader *shader = &foliage->depth_shader;

    glUseProgram(shader->program);
    shader_set_mat4(shader, "u_view_projection", view_projection);
    shader_set_vec3(shader, "u_view_origin", v3_scale(sun_direction, SUN_DISTANCE));
    shader_set_vec3(shader, "u_camera_position", camera_position);

    draw_chunks(foliage);

    glUseProgram(0);
}

int foliage_init(Foliage *foliage)
{
    int kind;

    memset(foliage, 0, sizeof *foliage);
    if (shader_load(&foliage->shader, "foliage") != 0 || shader_load(&foliage->depth_shader, "foliage_depth") != 0) {
        return -1;
    }
    for (kind = 0; kind < FOLIAGE_KINDS; kind++) {
        foliage->images[kind] = material_texture(kinds[kind].image, TEXTURE_CUTOUT);
        if (foliage->images[kind] == 0) {
            return -1;
        }
    }

    glUseProgram(foliage->shader.program);
    shader_set_int(&foliage->shader, "u_image", 0);
    shader_set_int(&foliage->shader, "u_shadow_map", SHADOW_UNIT);
    shader_set_float(&foliage->shader, "u_shadow_texel", SHADOW_TAP_SPREAD / SHADOW_MAP_SIZE);
    shader_set_float(&foliage->shader, "u_alpha_cutoff", ALPHA_CUTOFF);
    shader_set_float(&foliage->shader, "u_fade_end", FADE_END);
    shader_set_float(&foliage->shader, "u_fade_per_metre", FADE_PER_METRE);
    glUseProgram(foliage->depth_shader.program);
    shader_set_int(&foliage->depth_shader, "u_image", 0);
    shader_set_float(&foliage->depth_shader, "u_alpha_cutoff", ALPHA_CUTOFF);
    shader_set_float(&foliage->depth_shader, "u_fade_end", FADE_END);
    shader_set_float(&foliage->depth_shader, "u_fade_per_metre", FADE_PER_METRE);
    glUseProgram(0);

    return 0;
}
