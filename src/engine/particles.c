#include "engine/particles.h"

#include "engine/gl_ext.h"
#include "engine/image.h"
#include "engine/texture.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// share of the life a sprite takes to come up to its full alpha; the rest of it fades back out
#define FADE_IN 0.1f

static const float quad_corners[6][2] = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f},
                                         {-0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}};

typedef struct {
    float key;
    int index;
} Ordered;

int particles_init(Particles *particles)
{
    memset(particles, 0, sizeof *particles);
    if (shader_load(&particles->shader, "particle") != 0) {
        return -1;
    }
    glGenBuffers(1, &particles->buffer);
    glUseProgram(particles->shader.program);
    shader_set_int(&particles->shader, "u_sheet", 0);
    glUseProgram(0);

    return 0;
}

int particles_sheet(Particles *particles, const char *const *frames, int count, int columns, TextureKind kind)
{
    ParticleSheet *sheet;
    Image atlas;
    const char *reason;

    if (particles->sheet_count == PARTICLES_SHEETS) {
        fprintf(stderr, "no room for another particle sheet: %s\n", frames[0]);
        return -1;
    }
    if (image_atlas(&atlas, frames, count, columns, &reason) != 0) {
        fprintf(stderr, "cannot build the sheet around %s: %s\n", frames[0], reason);
        return -1;
    }

    sheet = &particles->sheets[particles->sheet_count];
    sheet->texture = texture_create(&atlas, kind, frames[0]);
    image_free(&atlas);
    if (sheet->texture == 0) {
        return -1;
    }
    sheet->columns = columns;
    sheet->rows = (count + columns - 1) / columns;
    sheet->frames = count;

    return particles->sheet_count++;
}

void particles_clear(Particles *particles)
{
    int i;

    for (i = 0; i < PARTICLES_MAX; i++) {
        particles->pool[i].lifetime = 0.0f;
    }
}

Particle *particles_spawn(Particles *particles)
{
    Particle *oldest = &particles->pool[0];
    int i;

    for (i = 0; i < PARTICLES_MAX; i++) {
        Particle *particle = &particles->pool[i];

        if (particle->lifetime <= 0.0f) {
            oldest = particle;
            break;
        }
        if (particle->age * oldest->lifetime > oldest->age * particle->lifetime) {
            oldest = particle;
        }
    }

    memset(oldest, 0, sizeof *oldest);
    // marks the slot in use right away, so a second spawn before this one is filled won't reuse it
    oldest->lifetime = 1.0f;

    return oldest;
}

void particles_step(Particles *particles, float dt)
{
    int i;

    for (i = 0; i < PARTICLES_MAX; i++) {
        Particle *particle = &particles->pool[i];

        if (particle->lifetime <= 0.0f) {
            continue;
        }
        particle->age += dt;
        if (particle->age >= particle->lifetime) {
            particle->lifetime = 0.0f;
            continue;
        }
        particle->position = v3_add(particle->position, v3_scale(particle->velocity, dt));
        particle->size += particle->growth * dt;
        particle->rotation += particle->spin * dt;
    }
}

int particles_frame(const Particles *particles, const Particle *particle)
{
    const ParticleSheet *sheet = &particles->sheets[particle->sheet];
    const int frame = particle->frame + (int)(particle->age * particle->frame_rate);

    return frame < sheet->frames - 1 ? frame : sheet->frames - 1;
}

static int by_key(const void *a, const void *b)
{
    const Ordered *first = a;
    const Ordered *second = b;

    if (first->key != second->key) {
        return first->key < second->key ? -1 : 1;
    }

    return first->index - second->index;
}

int particles_order(const Particles *particles, Vec3 eye, int *order)
{
    Ordered alpha[PARTICLES_MAX];
    Ordered additive[PARTICLES_MAX];
    int alpha_count = 0;
    int additive_count = 0;
    int i;

    for (i = 0; i < PARTICLES_MAX; i++) {
        const Particle *particle = &particles->pool[i];

        if (particle->lifetime <= 0.0f) {
            continue;
        }
        if (particle->blend == PARTICLE_ALPHA) {
            // the furthest sprite goes first, so the nearer ones compose over it
            alpha[alpha_count].key = -v3_length(v3_sub(particle->position, eye));
            alpha[alpha_count].index = i;
            alpha_count++;
        } else {
            // adding is the same whatever the order, so these only need to be gathered by their sheet
            additive[additive_count].key = (float)particle->sheet;
            additive[additive_count].index = i;
            additive_count++;
        }
    }

    qsort(alpha, (size_t)alpha_count, sizeof *alpha, by_key);
    qsort(additive, (size_t)additive_count, sizeof *additive, by_key);
    for (i = 0; i < alpha_count; i++) {
        order[i] = alpha[i].index;
    }
    for (i = 0; i < additive_count; i++) {
        order[alpha_count + i] = additive[i].index;
    }

    return alpha_count + additive_count;
}

static void write_sprite(const Particles *particles, const Particle *particle, ParticleVertex *vertex)
{
    const ParticleSheet *sheet = &particles->sheets[particle->sheet];
    const int frame = particles_frame(particles, particle);
    const float u = (float)(frame % sheet->columns) / (float)sheet->columns;
    const float v = (float)(frame / sheet->columns) / (float)sheet->rows;
    const float share = particle->age / particle->lifetime;
    const float fade = share < FADE_IN ? share / FADE_IN : (1.0f - share) / (1.0f - FADE_IN);
    const Vec3 color = v3_lerp(particle->color, particle->fade_color, share);
    int corner;

    for (corner = 0; corner < 6; corner++) {
        vertex[corner].position = particle->position;
        vertex[corner].u = u + (quad_corners[corner][0] + 0.5f) / (float)sheet->columns;
        vertex[corner].v = v + (quad_corners[corner][1] + 0.5f) / (float)sheet->rows;
        vertex[corner].corner_x = quad_corners[corner][0];
        vertex[corner].corner_y = quad_corners[corner][1];
        vertex[corner].size = particle->size;
        vertex[corner].rotation = particle->rotation;
        vertex[corner].color = v4(color.x, color.y, color.z, particle->alpha * fade);
    }
}

static void point_attribute(GLuint index, GLint components, size_t offset)
{
    glVertexAttribPointer(index, components, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                          (const void *)(uintptr_t)offset);
    glEnableVertexAttribArray(index);
}

static void draw_run(Particles *particles, const Particle *particle, int first, int count)
{
    const int lit = particle->blend == PARTICLE_ALPHA;

    glBlendFunc(GL_SRC_ALPHA, lit ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE);
    shader_set_int(&particles->shader, "u_lit", lit);
    glBindTexture(GL_TEXTURE_2D, particles->sheets[particle->sheet].texture);
    glDrawArrays(GL_TRIANGLES, first * 6, count * 6);
}

void particles_draw(Particles *particles, Mat4 view_projection, const Camera *camera, const Light *light)
{
    const int count = particles_order(particles, camera->eye, particles->order);
    const Vec3 forward = v3_normalize(v3_sub(camera->target, camera->eye));
    const Vec3 right = v3_normalize(v3_cross(forward, camera->up));
    int first = 0;
    int i;

    if (count == 0) {
        return;
    }
    for (i = 0; i < count; i++) {
        write_sprite(particles, &particles->pool[particles->order[i]], &particles->vertices[i * 6]);
    }

    glUseProgram(particles->shader.program);
    shader_set_mat4(&particles->shader, "u_view_projection", view_projection);
    shader_set_vec3(&particles->shader, "u_right", right);
    shader_set_vec3(&particles->shader, "u_up", v3_cross(right, forward));
    shader_set_vec3(&particles->shader, "u_camera_position", camera->eye);
    light_apply(light, &particles->shader);

    glBindBuffer(GL_ARRAY_BUFFER, particles->buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof *particles->vertices * (size_t)count * 6),
                 particles->vertices, GL_STREAM_DRAW);
    point_attribute(SHADER_ATTRIBUTE_POSITION, 3, offsetof(ParticleVertex, position));
    point_attribute(SHADER_ATTRIBUTE_UV, 2, offsetof(ParticleVertex, u));
    point_attribute(SHADER_ATTRIBUTE_CORNER, 4, offsetof(ParticleVertex, corner_x));
    point_attribute(SHADER_ATTRIBUTE_COLOR, 4, offsetof(ParticleVertex, color));

    glEnable(GL_BLEND);
    // particles are depth-tested against the scene but write no depth of their own; culling is off since a
    // billboard has no back face
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    for (i = 1; i <= count; i++) {
        const Particle *particle = &particles->pool[particles->order[first]];

        if (i < count && particles->pool[particles->order[i]].sheet == particle->sheet &&
            particles->pool[particles->order[i]].blend == particle->blend) {
            continue;
        }
        draw_run(particles, particle, first, i - first);
        first = i;
    }
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_COLOR);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_CORNER);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_UV);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
