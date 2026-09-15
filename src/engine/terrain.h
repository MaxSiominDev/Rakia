#ifndef TERRAIN_H
#define TERRAIN_H

#include "engine/camera.h"
#include "engine/gl_compat.h"
#include "engine/light.h"
#include "engine/mesh.h"
#include "engine/mesh_data.h"
#include "engine/shader.h"
#include "engine/shadow.h"
#include "engine/vecmath.h"

// the airbase stands on a patch flattened to y = 0, long enough for the 2500 m runway along z
#define TERRAIN_BASE_HALF_WIDTH 800.0f
#define TERRAIN_BASE_HALF_LENGTH 1400.0f
// the land west of the coast falls below this
#define TERRAIN_SEA_LEVEL (-30.0f)
#define TERRAIN_VIEW_DISTANCE 8000.0f

// each set is a diffuse, a normal and a roughness map
#define TERRAIN_TEXTURE_SETS 4

// the finest chunk is this wide and every level doubles the size and the vertex spacing
#define TERRAIN_CHUNK_SIZE 512.0f
#define TERRAIN_CHUNK_QUADS 64
#define TERRAIN_LEVELS 4
// cells across one level, even so that the block cut out for the finer level lands on cell borders
#define TERRAIN_LEVEL_CELLS 6
// every level but the finest drops the three by three block the level below already covers
#define TERRAIN_VISIBLE_MAX (TERRAIN_LEVEL_CELLS * TERRAIN_LEVEL_CELLS * TERRAIN_LEVELS - (TERRAIN_LEVELS - 1) * 9)
// the spare slots keep the chunks behind the camera around while it turns
#define TERRAIN_CHUNK_SLOTS (TERRAIN_VISIBLE_MAX + 32)
#define TERRAIN_BUILD_BUDGET 2

typedef struct {
    float height;
    // 0 in the desert, 1 in the green north
    float green;
    // 0 on the dunes, 1 where the rocky hills rise
    float rock;
} TerrainPoint;

TerrainPoint terrain_sample(float x, float z);
int terrain_on_base(float x, float z);
float terrain_height(float x, float z);
Vec3 terrain_normal(float x, float z);
// the grid of a chunk, its corner at (ix, iz) counted in cells of the level's size
int terrain_chunk_data(MeshData *data, int level, int ix, int iz);

typedef struct {
    int level;
    int ix;
    int iz;
    // frame the chunk was last wanted; the oldest one is evicted when the slots run out
    int touched;
    Mesh mesh;
} TerrainChunk;

typedef struct {
    Shader shader;
    GLuint maps[TERRAIN_TEXTURE_SETS * 3];
    TerrainChunk chunks[TERRAIN_CHUNK_SLOTS];
    int chunk_count;
    int visible[TERRAIN_VISIBLE_MAX];
    int visible_count;
    int frame;
} Terrain;

int terrain_init(Terrain *terrain);
void terrain_update(Terrain *terrain, Vec3 center);
void terrain_draw(Terrain *terrain, Mat4 view_projection, const Camera *camera, const Light *light,
                  const Shadow *shadow);
void terrain_draw_depth(Terrain *terrain, Shader *depth, Mat4 view_projection, Vec3 center, float radius);

#endif
