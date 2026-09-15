#ifndef FOLIAGE_H
#define FOLIAGE_H

#include "engine/camera.h"
#include "engine/gl_compat.h"
#include "engine/light.h"
#include "engine/shader.h"
#include "engine/shadow.h"
#include "engine/vecmath.h"

#define FOLIAGE_KINDS 4
// plants are placed in square cells of this size, at most this many to a cell
#define FOLIAGE_CELL 1024.0f
#define FOLIAGE_PER_CELL 1400
#define FOLIAGE_CHUNK_SLOTS 36

typedef struct {
    Vec3 position;
    float half_width;
    float height;
    int kind;
} FoliagePlant;

// the plants of one cell, from its index alone, so nothing has to be stored between runs
int foliage_place(int ix, int iz, FoliagePlant *plants, int max);

typedef struct {
    int ix;
    int iz;
    int touched;
    GLuint buffer;
    int first[FOLIAGE_KINDS];
    int count[FOLIAGE_KINDS];
} FoliageChunk;

typedef struct {
    Shader shader;
    Shader depth_shader;
    GLuint images[FOLIAGE_KINDS];
    FoliageChunk chunks[FOLIAGE_CHUNK_SLOTS];
    int chunk_count;
    int visible[FOLIAGE_CHUNK_SLOTS];
    int visible_count;
    int frame;
} Foliage;

int foliage_init(Foliage *foliage);
void foliage_update(Foliage *foliage, Vec3 center);
void foliage_draw(Foliage *foliage, Mat4 view_projection, const Camera *camera, const Light *light,
                  const Shadow *shadow);
// billboards turn toward the sun instead of the camera, which gives them a tree-shaped shadow
void foliage_draw_depth(Foliage *foliage, Mat4 view_projection, Vec3 camera_position, Vec3 sun_direction);

#endif
