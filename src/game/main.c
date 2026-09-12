#include "engine/assets.h"
#include "engine/camera.h"
#include "engine/input.h"
#include "engine/light.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh_data.h"
#include "engine/obj.h"
#include "engine/options.h"
#include "engine/platform.h"
#include "engine/renderer.h"
#include "engine/scene.h"
#include "engine/vecmath.h"

#include <stdio.h>
#include <stdlib.h>

#define KEY_ESCAPE 27

#define JET_MODEL "raw/aircraft/f16_rickslash/f16_rickslash.obj"
#define JET_SCALE 7.5f
#define SKY_PANORAMA "raw/sky/belfast_sunset_puresky/belfast_sunset_puresky_2k.hdr"
#define APRON_TEXTURE(map) "raw/textures/apron_concrete_concrete_pavement/concrete_pavement_" map
#define APRON_SIZE 200.0f
#define APRON_TILE 1.8f
#define SHADOW_RADIUS 20.0f

#define VIEW_YAW (35.0f * VEC_DEGREES)
#define VIEW_PITCH (8.0f * VEC_DEGREES)
#define VIEW_DISTANCE 22.0f
#define VIEW_MIN_PITCH (3.0f * VEC_DEGREES)
#define VIEW_MAX_PITCH (85.0f * VEC_DEGREES)
#define VIEW_MIN_DISTANCE 8.0f
#define VIEW_MAX_DISTANCE 150.0f
#define ORBIT_RATE (60.0f * VEC_DEGREES)
#define ZOOM_RATE 1.6f

static const char *const canopy_groups[] = {"cab_keep", "cab_around"};
#define CANOPY_GROUP_COUNT ((int)(sizeof canopy_groups / sizeof canopy_groups[0]))

typedef struct {
    Renderer renderer;
    Scene scene;
    Camera camera;
    Light light;
    Mesh jet_mesh;
    Material *jet_materials;
    Mesh apron_mesh;
    Material apron_material;
    Vec3 jet_center;
    float view_yaw;
    float view_pitch;
    float view_distance;
} Game;

static int asset(char *out, const char *relative)
{
    if (assets_path(out, ASSETS_PATH_MAX, relative) != 0) {
        fprintf(stderr, "asset path is too long: %s\n", relative);
        return -1;
    }

    return 0;
}

static int load_jet(Game *game)
{
    char path[ASSETS_PATH_MAX];
    Model model;
    Entity *jet;
    int i;

    if (asset(path, JET_MODEL) != 0 || obj_load(&model, path) != 0) {
        return -1;
    }
    game->jet_materials = malloc(sizeof *game->jet_materials * (size_t)model.material_count);
    if (game->jet_materials == NULL || mesh_create(&game->jet_mesh, &model.data) != 0) {
        fprintf(stderr, "out of memory loading %s\n", path);
        obj_free(&model);
        return -1;
    }
    for (i = 0; i < model.material_count; i++) {
        material_load(&game->jet_materials[i], &model.materials[i], model.directory);
    }
    obj_free(&model);

    jet = scene_add(&game->scene, &game->jet_mesh, game->jet_materials);
    if (jet == NULL) {
        fprintf(stderr, "%s has %d groups, more than an entity can carry\n", JET_MODEL, game->jet_mesh.group_count);
        return -1;
    }
    jet->scale = JET_SCALE;
    for (i = 0; i < CANOPY_GROUP_COUNT; i++) {
        const int group = mesh_group_index(&game->jet_mesh, canopy_groups[i]);

        if (group < 0) {
            fprintf(stderr, "%s has no group %s, the canopy will be drawn opaque\n", JET_MODEL, canopy_groups[i]);
            continue;
        }
        jet->group_flags[group] = GROUP_BLENDED | GROUP_DOUBLE_SIDED;
    }
    game->jet_center = v3_scale(v3_lerp(game->jet_mesh.bounds_min, game->jet_mesh.bounds_max, 0.5f), JET_SCALE);

    return 0;
}

static int load_apron(Game *game)
{
    char diffuse[ASSETS_PATH_MAX];
    char normal[ASSETS_PATH_MAX];
    char roughness[ASSETS_PATH_MAX];
    MeshData quad;
    Entity *apron;

    if (asset(diffuse, APRON_TEXTURE("diffuse_2k.jpg")) != 0 || asset(normal, APRON_TEXTURE("normal_gl_1k.jpg")) != 0 ||
        asset(roughness, APRON_TEXTURE("roughness_1k.jpg")) != 0) {
        return -1;
    }
    material_default(&game->apron_material);
    game->apron_material.diffuse_map = material_texture(diffuse, TEXTURE_COLOR);
    game->apron_material.normal_map = material_texture(normal, TEXTURE_DATA);
    // a gray roughness image has the roughness in every channel and fits the glTF packing as it is
    game->apron_material.metal_rough_map = material_texture(roughness, TEXTURE_DATA);
    if (game->apron_material.diffuse_map == 0 || game->apron_material.normal_map == 0 ||
        game->apron_material.metal_rough_map == 0) {
        return -1;
    }

    if (mesh_data_quad(&quad, APRON_SIZE, APRON_TILE) != 0 || mesh_create(&game->apron_mesh, &quad) != 0) {
        fprintf(stderr, "out of memory building the apron\n");
        return -1;
    }
    mesh_data_free(&quad);

    apron = scene_add(&game->scene, &game->apron_mesh, &game->apron_material);
    apron->casts_shadow = 0;

    return 0;
}

static void reset_view(Game *game)
{
    game->view_yaw = VIEW_YAW;
    game->view_pitch = VIEW_PITCH;
    game->view_distance = VIEW_DISTANCE;
}

static int game_init(void *context)
{
    Game *game = context;
    char panorama[ASSETS_PATH_MAX];

    if (asset(panorama, SKY_PANORAMA) != 0 || renderer_init(&game->renderer, panorama) != 0 ||
        load_jet(game) != 0 || load_apron(game) != 0) {
        return -1;
    }

    game->light = light_golden_hour();
    game->renderer.shadow_center = game->jet_center;
    game->renderer.shadow_radius = SHADOW_RADIUS;
    game->camera.fov_y_radians = 45.0f * VEC_DEGREES;
    game->camera.near_plane = 0.5f;
    game->camera.far_plane = 1000.0f;
    reset_view(game);

    return 0;
}

static float clamped(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}

static void game_update(void *context, const Input *input, float dt)
{
    Game *game = context;
    const float turn = ORBIT_RATE * dt;
    const int orbit = input_special_is_down(input, GLUT_KEY_LEFT) - input_special_is_down(input, GLUT_KEY_RIGHT);
    const int tilt = input_special_is_down(input, GLUT_KEY_UP) - input_special_is_down(input, GLUT_KEY_DOWN);

    // until the game has a pause state, Esc simply ends the program
    if (input_key_is_down(input, KEY_ESCAPE)) {
        exit(EXIT_SUCCESS);
    }
    if (input_key_is_down(input, 'r')) {
        reset_view(game);
    }

    game->view_yaw += turn * (float)orbit;
    game->view_pitch = clamped(game->view_pitch + turn * (float)tilt, VIEW_MIN_PITCH, VIEW_MAX_PITCH);
    if (input_key_is_down(input, 'w')) {
        game->view_distance /= 1.0f + (ZOOM_RATE - 1.0f) * dt;
    }
    if (input_key_is_down(input, 's')) {
        game->view_distance *= 1.0f + (ZOOM_RATE - 1.0f) * dt;
    }
    game->view_distance = clamped(game->view_distance, VIEW_MIN_DISTANCE, VIEW_MAX_DISTANCE);
}

static void game_render(void *context, int width, int height)
{
    Game *game = context;

    camera_orbit(&game->camera, game->jet_center, game->view_yaw, game->view_pitch, game->view_distance);
    renderer_draw(&game->renderer, &game->scene, &game->camera, &game->light, width, height);
}

int main(int argc, char **argv)
{
    static Game game;
    Options options;
    PlatformApp app;
    char error[256];

    if (options_parse(&options, argc, argv, error, sizeof error) != 0) {
        fprintf(stderr, "%s\n", error);
        options_print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }
    if (options.help) {
        options_print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
    }
    assets_init(options.assets_dir);

    app.context = &game;
    app.init = game_init;
    app.update = game_update;
    app.render = game_render;

    return platform_run(&options, &app, &argc, argv);
}
