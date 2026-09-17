#include "engine/terrain.h"

#include "engine/gl_ext.h"
#include "engine/material.h"
#include "engine/noise.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TERRAIN_SEED 4271u
#define SHADOW_UNIT (TERRAIN_TEXTURE_SETS * 3)

#define SEA_FLOOR (-95.0f)
// the coastline runs north to south this far west of the base, with the noise below waving it about
#define COAST_DISTANCE 2800.0f
#define COAST_WIDTH 1400.0f
#define COAST_WAVE 900.0f
// width of the ring the flat patch blends out over, in units of the patch
#define BASE_BLEND 0.45f
// the land turns green between these two distances north of the base
#define GREEN_START 2200.0f
#define GREEN_END 4800.0f
#define GREEN_WAVE 1500.0f
#define DUNE_HEIGHT 38.0f
#define HILL_HEIGHT 260.0f
// glPolygonOffset for the terrain in the depth pass: the sun is low, so the slope term does the work
#define DEPTH_SLOPE_BIAS 2.5f
#define DEPTH_BIAS 4.0f

// the vertex grid plus one sample of margin on each side, so every vertex normal has its neighbors
#define GRID_SIDE (TERRAIN_CHUNK_QUADS + 3)
#define VERTEX_SIDE (TERRAIN_CHUNK_QUADS + 1)
#define CHUNK_VERTICES (VERTEX_SIDE * VERTEX_SIDE + 4 * VERTEX_SIDE)
#define CHUNK_INDICES (TERRAIN_CHUNK_QUADS * TERRAIN_CHUNK_QUADS * 6 + 4 * TERRAIN_CHUNK_QUADS * 6)
// how far the border skirt hangs below the edge, in cells
#define SKIRT_CELLS 3.0f

#define TERRAIN_SET(dir, name) \
    {"raw/textures/" dir "/" name "_diffuse_2k.jpg", \
     "raw/textures/" dir "/" name "_normal_gl_1k.jpg", \
     "raw/textures/" dir "/" name "_roughness_1k.jpg"}

static const char *const map_uniforms[3] = {"u_diffuse", "u_normal", "u_rough"};

static const char *const texture_sets[TERRAIN_TEXTURE_SETS][3] = {
    TERRAIN_SET("sand_dunes_Ground097", "Ground097"),
    TERRAIN_SET("rocky_ground_rocky_trail_02", "rocky_trail_02"),
    TERRAIN_SET("rock_cliff_Rock029", "Rock029"),
    TERRAIN_SET("green_grass_Grass004", "Grass004"),
    TERRAIN_SET("dry_grass_withered_grass", "withered_grass")
};

static float smooth_step(float edge0, float edge1, float value)
{
    const float t = clamped((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);

    return t * t * (3.0f - 2.0f * t);
}

// distance from the middle of the airbase in units of the flat patch, 1 on its rim
static float base_distance(float x, float z)
{
    const float across = x / TERRAIN_BASE_HALF_WIDTH;
    const float along = z / TERRAIN_BASE_HALF_LENGTH;

    return sqrtf(across * across + along * along);
}

int terrain_on_base(float x, float z)
{
    return base_distance(x, z) <= 1.0f;
}

TerrainPoint terrain_sample(float x, float z)
{
    float wx = x;
    float wz = z;
    TerrainPoint point;
    float rocky;
    float dunes;
    float ridges;
    float hills;
    float land;
    float flat;
    float coast;

    // the warp bends the ridges into wadis and stops the hills from looking like a noise grid
    noise_warp(&wx, &wz, 1.0f / 3100.0f, 240.0f, TERRAIN_SEED + 11u);

    point.green = smooth_step(GREEN_START, GREEN_END,
                              z + GREEN_WAVE * noise2(x / 5200.0f, 0.37f, TERRAIN_SEED + 3u));
    rocky = smooth_step(0.35f, 0.70f,
                        noise_fbm(wx / 6100.0f, wz / 6100.0f, TERRAIN_SEED + 5u, 3, 2.0f, 0.5f) * 0.5f + 0.5f);
    rocky *= 1.0f - 0.55f * point.green;

    dunes = (noise_fbm(wx / 700.0f, wz / 700.0f, TERRAIN_SEED + 7u, 6, 2.1f, 0.5f) * 0.5f + 0.5f) * DUNE_HEIGHT;
    ridges = noise_ridged(wx / 2400.0f, wz / 2400.0f, TERRAIN_SEED + 13u, 4, 2.2f, 0.5f);
    // a rocky region is a raised plateau with the ridges cut into it, which is how the Negev reads
    hills = (0.25f + 0.75f * ridges * ridges) * HILL_HEIGHT * rocky;

    land = dunes + hills;
    flat = 1.0f - smooth_step(1.0f, 1.0f + BASE_BLEND, base_distance(x, z));
    land *= 1.0f - flat;

    coast = smooth_step(-COAST_WIDTH, COAST_WIDTH,
                        x - COAST_DISTANCE + COAST_WAVE * noise2(z / 2600.0f, 4.1f, TERRAIN_SEED + 17u));
    point.height = land * (1.0f - coast) +
                   (SEA_FLOOR + 12.0f * noise2(wx / 1700.0f, wz / 1700.0f, TERRAIN_SEED + 19u)) * coast;
    point.green *= 1.0f - coast;
    // the stony ground covers the crests and thins out over the rest of the rocky region
    point.rock = fmaxf(smooth_step(40.0f, 140.0f, hills), 0.45f * rocky) * (1.0f - coast) * (1.0f - flat);

    return point;
}

float terrain_height(float x, float z)
{
    return terrain_sample(x, z).height;
}

Vec3 terrain_normal(float x, float z)
{
    const float step = 1.0f;
    const float dx = terrain_height(x + step, z) - terrain_height(x - step, z);
    const float dz = terrain_height(x, z + step) - terrain_height(x, z - step);

    return v3_normalize(v3(-dx / (2.0f * step), 1.0f, -dz / (2.0f * step)));
}

static float level_size(int level)
{
    return TERRAIN_CHUNK_SIZE * (float)(1 << level);
}

static void quad_indices(unsigned int *index, unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    index[0] = a;
    index[1] = b;
    index[2] = c;
    index[3] = a;
    index[4] = c;
    index[5] = d;
}

// walked so that the quads hanging off each edge face away from the chunk
static const struct {
    int start_i;
    int start_j;
    int step_i;
    int step_j;
} skirt_edges[4] = {
    {0, 0, 0, 1},
    {TERRAIN_CHUNK_QUADS, TERRAIN_CHUNK_QUADS, 0, -1},
    {TERRAIN_CHUNK_QUADS, 0, -1, 0},
    {0, TERRAIN_CHUNK_QUADS, 1, 0}
};

int terrain_chunk_data(MeshData *data, int level, int ix, int iz)
{
    const float size = level_size(level);
    const float spacing = size / TERRAIN_CHUNK_QUADS;
    const float origin_x = (float)ix * size;
    const float origin_z = (float)iz * size;
    TerrainPoint *grid = malloc(sizeof *grid * GRID_SIDE * GRID_SIDE);
    int i;
    int j;
    int edge;
    int index = 0;

    memset(data, 0, sizeof *data);
    data->vertices = malloc(sizeof *data->vertices * CHUNK_VERTICES);
    data->indices = malloc(sizeof *data->indices * CHUNK_INDICES);
    data->groups = calloc(1, sizeof *data->groups);
    if (grid == NULL || data->vertices == NULL || data->indices == NULL || data->groups == NULL) {
        free(grid);
        mesh_data_free(data);
        return -1;
    }
    data->vertex_count = CHUNK_VERTICES;
    data->index_count = CHUNK_INDICES;
    data->group_count = 1;
    data->groups[0].index_count = CHUNK_INDICES;

    for (j = 0; j < GRID_SIDE; j++) {
        for (i = 0; i < GRID_SIDE; i++) {
            grid[j * GRID_SIDE + i] = terrain_sample(origin_x + (float)(i - 1) * spacing,
                                                     origin_z + (float)(j - 1) * spacing);
        }
    }

    for (j = 0; j <= TERRAIN_CHUNK_QUADS; j++) {
        for (i = 0; i <= TERRAIN_CHUNK_QUADS; i++) {
            const TerrainPoint *point = &grid[(j + 1) * GRID_SIDE + i + 1];
            const float dx = (point[1].height - point[-1].height) / (2.0f * spacing);
            const float dz = (point[GRID_SIDE].height - point[-GRID_SIDE].height) / (2.0f * spacing);
            MeshVertex *vertex = &data->vertices[j * VERTEX_SIDE + i];

            vertex->position = v3(origin_x + (float)i * spacing, point->height, origin_z + (float)j * spacing);
            vertex->normal = v3_normalize(v3(-dx, 1.0f, -dz));
            // the shader takes its texture coordinates from the world position, so the uv slot carries
            // the two blend weights instead
            vertex->u = point->green;
            vertex->v = point->rock;
            vertex->tangent = v3_normalize(v3(1.0f, dx, 0.0f));
            vertex->tangent_sign = 1.0f;
        }
    }

    for (j = 0; j < TERRAIN_CHUNK_QUADS; j++) {
        for (i = 0; i < TERRAIN_CHUNK_QUADS; i++) {
            const unsigned int corner = (unsigned int)(j * VERTEX_SIDE + i);

            quad_indices(&data->indices[index], corner, corner + VERTEX_SIDE,
                         corner + VERTEX_SIDE + 1, corner + 1);
            index += 6;
        }
    }

    for (edge = 0; edge < 4; edge++) {
        const int first = VERTEX_SIDE * VERTEX_SIDE + edge * VERTEX_SIDE;
        int step;

        for (step = 0; step <= TERRAIN_CHUNK_QUADS; step++) {
            const int i_grid = skirt_edges[edge].start_i + skirt_edges[edge].step_i * step;
            const int j_grid = skirt_edges[edge].start_j + skirt_edges[edge].step_j * step;
            const MeshVertex *top = &data->vertices[j_grid * VERTEX_SIDE + i_grid];
            MeshVertex *vertex = &data->vertices[first + step];

            *vertex = *top;
            vertex->position.y -= spacing * SKIRT_CELLS;
            if (step < TERRAIN_CHUNK_QUADS) {
                const int next_i = i_grid + skirt_edges[edge].step_i;
                const int next_j = j_grid + skirt_edges[edge].step_j;

                quad_indices(&data->indices[index], (unsigned int)(j_grid * VERTEX_SIDE + i_grid),
                             (unsigned int)(first + step), (unsigned int)(first + step + 1),
                             (unsigned int)(next_j * VERTEX_SIDE + next_i));
                index += 6;
            }
        }
    }

    free(grid);

    return 0;
}

static int floor_index(float value, float size)
{
    return (int)floorf(value / size);
}

static int find_chunk(const Terrain *terrain, int level, int ix, int iz)
{
    int i;

    for (i = 0; i < terrain->chunk_count; i++) {
        const TerrainChunk *chunk = &terrain->chunks[i];

        if (chunk->level == level && chunk->ix == ix && chunk->iz == iz) {
            return i;
        }
    }

    return -1;
}

static int free_slot(Terrain *terrain)
{
    int oldest = 0;
    int i;

    if (terrain->chunk_count < TERRAIN_CHUNK_SLOTS) {
        return terrain->chunk_count++;
    }
    for (i = 1; i < terrain->chunk_count; i++) {
        if (terrain->chunks[i].touched < terrain->chunks[oldest].touched) {
            oldest = i;
        }
    }
    mesh_free(&terrain->chunks[oldest].mesh);

    return oldest;
}

static int build_chunk(Terrain *terrain, int level, int ix, int iz)
{
    MeshData data;
    Mesh mesh;
    TerrainChunk *chunk;
    int uploaded;
    int slot;

    if (terrain_chunk_data(&data, level, ix, iz) != 0) {
        return -1;
    }
    // the slot is taken only once the mesh is there, so a failure cannot evict a chunk for nothing
    uploaded = mesh_create(&mesh, &data);
    mesh_data_free(&data);
    if (uploaded != 0) {
        return -1;
    }

    slot = free_slot(terrain);
    chunk = &terrain->chunks[slot];
    chunk->mesh = mesh;
    chunk->level = level;
    chunk->ix = ix;
    chunk->iz = iz;

    return slot;
}

void terrain_update(Terrain *terrain, Vec3 center)
{
    // the first ring is built in one go so the opening frame is complete
    const int budget = terrain->chunk_count == 0 ? TERRAIN_VISIBLE_MAX : TERRAIN_BUILD_BUDGET;
    int built = 0;
    int level;

    terrain->frame++;
    terrain->visible_count = 0;
    for (level = 0; level < TERRAIN_LEVELS; level++) {
        const float size = level_size(level);
        const int anchor_x = 2 * floor_index(center.x, size * 2.0f);
        const int anchor_z = 2 * floor_index(center.z, size * 2.0f);
        const int covered_x = floor_index(center.x, size);
        const int covered_z = floor_index(center.z, size);
        int i;
        int j;

        for (j = 0; j < TERRAIN_LEVEL_CELLS; j++) {
            for (i = 0; i < TERRAIN_LEVEL_CELLS; i++) {
                const int ix = anchor_x - 2 + i;
                const int iz = anchor_z - 2 + j;
                int slot;

                if (level > 0 && abs(ix - covered_x) <= 1 && abs(iz - covered_z) <= 1) {
                    continue;
                }
                slot = find_chunk(terrain, level, ix, iz);
                if (slot < 0) {
                    if (built == budget) {
                        continue;
                    }
                    slot = build_chunk(terrain, level, ix, iz);
                    if (slot < 0) {
                        continue;
                    }
                    built++;
                }
                terrain->chunks[slot].touched = terrain->frame;
                terrain->visible[terrain->visible_count++] = slot;
            }
        }
    }
}

// the six clip planes of a view projection, each as a normal and a distance with the inside at n . p + d > 0
static void frustum_planes(Mat4 m, float planes[6][4])
{
    int plane;

    for (plane = 0; plane < 6; plane++) {
        const int row = plane / 2;
        const float sign = plane % 2 == 0 ? 1.0f : -1.0f;
        int i;

        for (i = 0; i < 4; i++) {
            planes[plane][i] = m.m[i * 4 + 3] + sign * m.m[i * 4 + row];
        }
    }
}

static int box_visible(const float planes[6][4], Vec3 min, Vec3 max)
{
    int plane;

    for (plane = 0; plane < 6; plane++) {
        const float *p = planes[plane];
        // the box corner reaching farthest along the plane normal decides
        const float x = p[0] > 0.0f ? max.x : min.x;
        const float y = p[1] > 0.0f ? max.y : min.y;
        const float z = p[2] > 0.0f ? max.z : min.z;

        if (p[0] * x + p[1] * y + p[2] * z + p[3] < 0.0f) {
            return 0;
        }
    }

    return 1;
}

static void draw_chunk(const TerrainChunk *chunk)
{
    mesh_bind(&chunk->mesh);
    mesh_draw_group(&chunk->mesh, 0);
    mesh_unbind();
}

static void bind_maps(const Terrain *terrain)
{
    int unit;

    for (unit = 0; unit < TERRAIN_TEXTURE_SETS * 3; unit++) {
        glActiveTexture(GL_TEXTURE0 + (GLenum)unit);
        glBindTexture(GL_TEXTURE_2D, terrain->maps[unit]);
    }
    glActiveTexture(GL_TEXTURE0);
}

void terrain_draw(Terrain *terrain, Mat4 view_projection, const Camera *camera, const Light *light,
                  const Shadow *shadow)
{
    Shader *shader = &terrain->shader;
    float planes[6][4];
    int unit;
    int i;

    frustum_planes(view_projection, planes);
    glUseProgram(shader->program);
    shader_set_mat4(shader, "u_view_projection", view_projection);
    shader_set_mat4(shader, "u_shadow_matrix", shadow->texture_matrix);
    shader_set_vec3(shader, "u_camera_position", camera->eye);
    light_apply(light, shader);
    bind_maps(terrain);
    glActiveTexture(GL_TEXTURE0 + SHADOW_UNIT);
    glBindTexture(GL_TEXTURE_2D, shadow->texture);
    glActiveTexture(GL_TEXTURE0);

    for (i = 0; i < terrain->visible_count; i++) {
        const TerrainChunk *chunk = &terrain->chunks[terrain->visible[i]];

        if (box_visible(planes, chunk->mesh.bounds_min, chunk->mesh.bounds_max)) {
            draw_chunk(chunk);
        }
    }

    for (unit = SHADOW_UNIT; unit >= 0; unit--) {
        glActiveTexture(GL_TEXTURE0 + (GLenum)unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

void terrain_draw_depth(Terrain *terrain, Shader *depth, Mat4 view_projection)
{
    float planes[6][4];
    int i;

    frustum_planes(view_projection, planes);
    glUseProgram(depth->program);
    shader_set_mat4(depth, "u_view_projection", view_projection);
    shader_set_mat4(depth, "u_model", m4_identity());
    shader_set_float(depth, "u_opacity", 1.0f);
    shader_set_float(depth, "u_alpha_cutoff", 0.0f);
    // an open surface has no back faces to fall back on, so it goes in front side first with a slope bias
    glCullFace(GL_BACK);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(DEPTH_SLOPE_BIAS, DEPTH_BIAS);

    for (i = 0; i < terrain->visible_count; i++) {
        const TerrainChunk *chunk = &terrain->chunks[terrain->visible[i]];

        if (box_visible(planes, chunk->mesh.bounds_min, chunk->mesh.bounds_max)) {
            draw_chunk(chunk);
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_FRONT);
    glUseProgram(0);
}

int terrain_init(Terrain *terrain)
{
    int set;
    int map;

    memset(terrain, 0, sizeof *terrain);
    if (shader_load(&terrain->shader, "terrain") != 0) {
        return -1;
    }
    glUseProgram(terrain->shader.program);
    for (set = 0; set < TERRAIN_TEXTURE_SETS; set++) {
        for (map = 0; map < 3; map++) {
            char name[SHADER_UNIFORM_NAME_LENGTH];
            const int unit = set * 3 + map;

            snprintf(name, sizeof name, "%s[%d]", map_uniforms[map], set);
            shader_set_int(&terrain->shader, name, unit);
            terrain->maps[unit] = material_texture(texture_sets[set][map], map == 0 ? TEXTURE_COLOR : TEXTURE_DATA);
            if (terrain->maps[unit] == 0) {
                return -1;
            }
        }
    }
    shader_set_int(&terrain->shader, "u_shadow_map", SHADOW_UNIT);
    shader_set_float(&terrain->shader, "u_shadow_texel", SHADOW_TAP_SPREAD / SHADOW_MAP_SIZE);
    glUseProgram(0);

    return 0;
}
