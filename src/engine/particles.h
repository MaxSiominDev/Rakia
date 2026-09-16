#ifndef PARTICLES_H
#define PARTICLES_H

#include "engine/camera.h"
#include "engine/gl_compat.h"
#include "engine/light.h"
#include "engine/shader.h"
#include "engine/texture.h"
#include "engine/vecmath.h"

#define PARTICLES_MAX 2048
#define PARTICLES_SHEETS 5

typedef enum {
    // smoke and fire compose over what is behind them; flashes only add their own light
    PARTICLE_ALPHA,
    PARTICLE_ADDITIVE
} ParticleBlend;

// one sprite standing in the world and facing the camera; a dead one has no lifetime left
typedef struct {
    Vec3 position;
    Vec3 velocity;
    // the sprite is multiplied by this, running from the first color to the second over the life
    Vec3 color;
    Vec3 fade_color;
    float alpha;
    // metres across, and how many metres a second it grows by
    float size;
    float growth;
    float rotation;
    float spin;
    float age;
    float lifetime;
    int sheet;
    // the frame it opens on and how many frames a second it runs through; 0 holds the one it opened on
    int frame;
    float frame_rate;
    ParticleBlend blend;
} Particle;

typedef struct {
    GLuint texture;
    int columns;
    int rows;
    int frames;
} ParticleSheet;

typedef struct {
    Vec3 position;
    float u;
    float v;
    // corner of the sprite in x and y, its size in z and its turn in w
    float corner_x;
    float corner_y;
    float size;
    float rotation;
    Vec4 color;
} ParticleVertex;

typedef struct {
    Shader shader;
    GLuint buffer;
    ParticleSheet sheets[PARTICLES_SHEETS];
    int sheet_count;
    Particle pool[PARTICLES_MAX];
    // the draw order and the vertices built from it, kept here so a frame allocates nothing
    int order[PARTICLES_MAX];
    ParticleVertex vertices[PARTICLES_MAX * 6];
} Particles;

int particles_init(Particles *particles);
// the frames stitched into one texture; the sheet number to put on a particle, -1 when it cannot be read
int particles_sheet(Particles *particles, const char *const *frames, int count, int columns, TextureKind kind);
void particles_clear(Particles *particles);
// a slot to fill in; the particle nearest its end gives way when the pool is full
Particle *particles_spawn(Particles *particles);
void particles_step(Particles *particles, float dt);
// which frame of its sheet a particle shows at its age
int particles_frame(const Particles *particles, const Particle *particle);
// the live particles in drawing order: the alpha ones from the back forward, then the additive ones by sheet
int particles_order(const Particles *particles, Vec3 eye, int *order);
void particles_draw(Particles *particles, Mat4 view_projection, const Camera *camera, const Light *light);

#endif
