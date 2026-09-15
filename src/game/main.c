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
#include "engine/terrain.h"
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
// the apron rests on the flattened base, just clear of the terrain it would otherwise fight for depth
#define APRON_LIFT 0.15f
#define JET_SHADOW_RADIUS 20.0f
#define FREE_SHADOW_RADIUS 800.0f
#define FREE_SHADOW_AHEAD 700.0f

#define VIEW_YAW (35.0f * VEC_DEGREES)
#define VIEW_PITCH (8.0f * VEC_DEGREES)
#define VIEW_DISTANCE 22.0f
#define VIEW_MIN_PITCH (3.0f * VEC_DEGREES)
#define VIEW_MAX_PITCH (85.0f * VEC_DEGREES)
#define VIEW_MIN_DISTANCE 8.0f
#define VIEW_MAX_DISTANCE 150.0f
#define ORBIT_RATE (60.0f * VEC_DEGREES)
#define ZOOM_RATE 1.6f

// the free camera opens 500 m up and south-east of the base, looking across the apron toward the green
// north, with the sea on the left
#define FREE_START_POSITION v3(-2200.0f, 500.0f, -1800.0f)
#define FREE_START_YAW (42.0f * VEC_DEGREES)
#define FREE_START_PITCH (-11.0f * VEC_DEGREES)
#define FREE_SPEED 50.0f
#define FREE_FAST_SPEED 500.0f
#define LOOK_RATE (60.0f * VEC_DEGREES)

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
    FlyCamera fly;
    int free_camera;
    int camera_key_down;
    float time;
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
    apron->position = v3(0.0f, APRON_LIFT, 0.0f);
    apron->casts_shadow = 0;

    return 0;
}

static void reset_view(Game *game)
{
    game->view_yaw = VIEW_YAW;
    game->view_pitch = VIEW_PITCH;
    game->view_distance = VIEW_DISTANCE;
    game->fly.position = FREE_START_POSITION;
    game->fly.yaw = FREE_START_YAW;
    game->fly.pitch = FREE_START_PITCH;
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
    game->camera.fov_y_radians = 45.0f * VEC_DEGREES;
    game->camera.near_plane = 1.0f;
    // the chunk ring reaches past the fog, and a nearer plane would fight for depth over the apron
    game->camera.far_plane = 4.0f * TERRAIN_VIEW_DISTANCE;
    game->free_camera = 1;
    reset_view(game);

    return 0;
}

static float clamped(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}

static void update_orbit(Game *game, const Input *input, float dt, int turn_left, int tilt_up)
{
    const float turn = ORBIT_RATE * dt;

    game->view_yaw += turn * (float)turn_left;
    game->view_pitch = clamped(game->view_pitch + turn * (float)tilt_up, VIEW_MIN_PITCH, VIEW_MAX_PITCH);
    if (input_key_is_down(input, 'w')) {
        game->view_distance /= 1.0f + (ZOOM_RATE - 1.0f) * dt;
    }
    if (input_key_is_down(input, 's')) {
        game->view_distance *= 1.0f + (ZOOM_RATE - 1.0f) * dt;
    }
    game->view_distance = clamped(game->view_distance, VIEW_MIN_DISTANCE, VIEW_MAX_DISTANCE);
}

static void update_fly(Game *game, const Input *input, float dt, int turn_left, int tilt_up)
{
    const float speed = (input_shift_is_down(input) ? FREE_FAST_SPEED : FREE_SPEED) * dt;
    const Vec3 move = v3((float)(input_key_is_down(input, 'd') - input_key_is_down(input, 'a')) * speed,
                         (float)(input_key_is_down(input, 'e') - input_key_is_down(input, 'q')) * speed,
                         (float)(input_key_is_down(input, 'w') - input_key_is_down(input, 's')) * speed);

    camera_fly_step(&game->fly, move, LOOK_RATE * dt * (float)turn_left, LOOK_RATE * dt * (float)tilt_up);
}

static void game_update(void *context, const Input *input, float dt)
{
    Game *game = context;
    const int turn_left = input_special_is_down(input, GLUT_KEY_LEFT) - input_special_is_down(input, GLUT_KEY_RIGHT);
    const int tilt_up = input_special_is_down(input, GLUT_KEY_UP) - input_special_is_down(input, GLUT_KEY_DOWN);
    const int camera_key = input_key_is_down(input, 'c');

    // until the game has a pause state, Esc simply ends the program
    if (input_key_is_down(input, KEY_ESCAPE)) {
        exit(EXIT_SUCCESS);
    }
    if (camera_key && !game->camera_key_down) {
        game->free_camera = !game->free_camera;
    }
    game->camera_key_down = camera_key;
    if (input_key_is_down(input, 'r')) {
        reset_view(game);
    }

    if (game->free_camera) {
        update_fly(game, input, dt, turn_left, tilt_up);
    } else {
        update_orbit(game, input, dt, turn_left, tilt_up);
    }
    game->time += dt;
}

// a map 1600 m across cannot hold the whole view, so it follows the ground the free camera looks at
static void aim_shadow(Game *game)
{
    const Vec3 view = v3_sub(game->camera.target, game->camera.eye);
    const Vec3 ahead = v3_add(game->camera.eye,
                              v3_scale(v3_normalize(v3(view.x, 0.0f, view.z)), FREE_SHADOW_AHEAD));

    game->renderer.shadow_center = v3(ahead.x, terrain_height(ahead.x, ahead.z), ahead.z);
    game->renderer.shadow_radius = FREE_SHADOW_RADIUS;
}

static void game_render(void *context, int width, int height)
{
    Game *game = context;

    if (game->free_camera) {
        camera_fly(&game->camera, &game->fly);
        aim_shadow(game);
    } else {
        camera_orbit(&game->camera, game->jet_center, game->view_yaw, game->view_pitch, game->view_distance);
        game->renderer.shadow_center = game->jet_center;
        game->renderer.shadow_radius = JET_SHADOW_RADIUS;
    }
    renderer_draw(&game->renderer, &game->scene, &game->camera, &game->light, game->time, width, height);
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
