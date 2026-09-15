#include "game/game.h"

#include "engine/assets.h"
#include "engine/mesh_data.h"
#include "engine/obj.h"
#include "engine/terrain.h"
#include "game/hud.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define KEY_ESCAPE 27
#define KEY_ENTER 13

#define JET_MODEL "raw/aircraft/f16_rickslash/f16_rickslash.obj"
#define JET_SCALE 7.5f
#define SKY_PANORAMA "raw/sky/belfast_sunset_puresky/belfast_sunset_puresky_2k.hdr"
#define APRON_TEXTURE(map) "raw/textures/apron_concrete_concrete_pavement/concrete_pavement_" map
#define APRON_SIZE 200.0f
#define APRON_TILE 1.8f
// the apron rests on the flattened base, just clear of the terrain it would otherwise fight for depth
#define APRON_LIFT 0.15f

// the flight opens south of the base heading north, so the base patch lies ahead in the chase view
#define START_X 0.0f
#define START_Z (-3500.0f)
#define START_HEADING 0.0f
#define START_HEIGHT 150.0f
#define START_SPEED 250.0f
#define START_THROTTLE 0.7f

// the sun is ten degrees up, so the jet's shadow lands more than two kilometres ahead of it: the map is fitted
// around the ground the sun ray through the jet reaches, and reaches back up to the jet itself
#define SHADOW_RADIUS 900.0f
// room for the dunes between the jet and its shadow
#define SHADOW_RELIEF 200.0f

#define VIEW_YAW (35.0f * VEC_DEGREES)
#define VIEW_PITCH (8.0f * VEC_DEGREES)
#define VIEW_DISTANCE 22.0f
#define VIEW_MIN_PITCH (3.0f * VEC_DEGREES)
#define VIEW_MAX_PITCH (85.0f * VEC_DEGREES)
#define VIEW_MIN_DISTANCE 8.0f
#define VIEW_MAX_DISTANCE 150.0f
#define ORBIT_RATE (60.0f * VEC_DEGREES)
#define ZOOM_RATE 1.6f

#define FREE_SPEED 50.0f
#define FREE_FAST_SPEED 500.0f
#define LOOK_RATE (60.0f * VEC_DEGREES)

#define INVERT_NOTE_SECONDS 1.5f

static const char *const canopy_groups[] = {"cab_keep", "cab_around"};
#define CANOPY_GROUP_COUNT ((int)(sizeof canopy_groups / sizeof canopy_groups[0]))

// the gear and its doors belong to a jet on the apron, not to one in the air
static const char *const gear_groups[] = {"chassis_container_front", "chassis_container_left",
                                          "chassis_container_right", "chassis_fold1", "chassis_fold2",
                                          "chassis_front_fold"};
#define GEAR_GROUP_COUNT ((int)(sizeof gear_groups / sizeof gear_groups[0]))

static int asset(char *out, const char *relative)
{
    if (assets_path(out, ASSETS_PATH_MAX, relative) != 0) {
        fprintf(stderr, "asset path is too long: %s\n", relative);
        return -1;
    }

    return 0;
}

static void flag_groups(Game *game, const char *const *names, int count, unsigned char flags)
{
    int i;

    for (i = 0; i < count; i++) {
        const int group = mesh_group_index(&game->jet_mesh, names[i]);

        if (group < 0) {
            fprintf(stderr, "%s has no group %s\n", JET_MODEL, names[i]);
            continue;
        }
        game->jet->group_flags[group] = flags;
    }
}

// the pilot sits inside the canopy, so its groups say where the cockpit view looks from; the box starts
// inverted, which leaves the eye in the middle of the jet if the model carries no canopy at all
static void measure_cockpit(Game *game, const Model *model)
{
    Vec3 min = game->jet_mesh.bounds_max;
    Vec3 max = game->jet_mesh.bounds_min;
    int i;

    for (i = 0; i < CANOPY_GROUP_COUNT; i++) {
        const int group = mesh_group_index(&game->jet_mesh, canopy_groups[i]);
        Vec3 group_min;
        Vec3 group_max;

        if (group < 0) {
            continue;
        }
        mesh_data_group_bounds(&model->data, group, &group_min, &group_max);
        min = v3(fminf(min.x, group_min.x), fminf(min.y, group_min.y), fminf(min.z, group_min.z));
        max = v3(fmaxf(max.x, group_max.x), fmaxf(max.y, group_max.y), fmaxf(max.z, group_max.z));
    }
    game->cockpit_eye = v3_scale(v3_lerp(min, max, 0.5f), JET_SCALE);
}

static int load_jet(Game *game)
{
    char path[ASSETS_PATH_MAX];
    Model model;
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

    game->jet = scene_add(&game->scene, &game->jet_mesh, game->jet_materials);
    if (game->jet == NULL) {
        fprintf(stderr, "%s has %d groups, more than an entity can carry\n", JET_MODEL, game->jet_mesh.group_count);
        obj_free(&model);
        return -1;
    }
    game->jet->scale = JET_SCALE;
    flag_groups(game, canopy_groups, CANOPY_GROUP_COUNT, GROUP_BLENDED | GROUP_DOUBLE_SIDED);
    flag_groups(game, gear_groups, GEAR_GROUP_COUNT, GROUP_HIDDEN);
    measure_cockpit(game, &model);
    obj_free(&model);

    game->jet_min = v3_scale(game->jet_mesh.bounds_min, JET_SCALE);
    game->jet_max = v3_scale(game->jet_mesh.bounds_max, JET_SCALE);
    game->jet_center = v3_lerp(game->jet_min, game->jet_max, 0.5f);

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

static void start_flight(Game *game)
{
    const Vec3 start = v3(START_X, terrain_height(START_X, START_Z) + START_HEIGHT, START_Z);

    aircraft_place(&game->aircraft, start, START_HEADING, START_SPEED, START_THROTTLE);
    camera_chase_settle(&game->chase, game->aircraft.orientation);
    game->time = 0.0f;
    game->state = GAME_FLIGHT;
}

int game_init(void *context)
{
    Game *game = context;
    char panorama[ASSETS_PATH_MAX];

    if (asset(panorama, SKY_PANORAMA) != 0 || renderer_init(&game->renderer, panorama) != 0 ||
        load_jet(game) != 0 || load_apron(game) != 0 || text_init(&game->hud) != 0) {
        return -1;
    }

    game->light = light_golden_hour();
    game->camera.fov_y_radians = 45.0f * VEC_DEGREES;
    game->camera.near_plane = 1.0f;
    // the chunk ring reaches past the fog, and a nearer plane would fight for depth over the apron
    game->camera.far_plane = 4.0f * TERRAIN_VIEW_DISTANCE;
    start_flight(game);

    return 0;
}

static int pressed(const Game *game, const Input *input, unsigned char key)
{
    return input_key_is_down(input, key) && !input_key_is_down(&game->previous, key);
}

// the eight corners of the jet's box, so a wing tip counts as much as the belly
static int touches_ground(const Game *game)
{
    const Mat4 model = m4_multiply(m4_translate(game->aircraft.position), quat_to_mat4(game->aircraft.orientation));
    int corner;

    for (corner = 0; corner < 8; corner++) {
        const Vec3 local = v3(corner & 1 ? game->jet_max.x : game->jet_min.x,
                              corner & 2 ? game->jet_max.y : game->jet_min.y,
                              corner & 4 ? game->jet_max.z : game->jet_min.z);
        const Vec3 world = m4_transform_point(model, local);

        if (world.y <= fmaxf(terrain_height(world.x, world.z), TERRAIN_SEA_LEVEL)) {
            return 1;
        }
    }

    return 0;
}

static void step_flight(Game *game, const Input *input, float dt)
{
    const int stick = input_special_is_down(input, GLUT_KEY_UP) - input_special_is_down(input, GLUT_KEY_DOWN);
    AircraftControls controls;

    controls.pitch = (float)(game->inverted_pitch ? -stick : stick);
    controls.bank = (float)(input_special_is_down(input, GLUT_KEY_RIGHT) -
                            input_special_is_down(input, GLUT_KEY_LEFT));
    controls.rudder = (float)(input_key_is_down(input, 'd') - input_key_is_down(input, 'a'));
    controls.afterburner = input_shift_is_down(input);

    if (pressed(game, input, 'w')) {
        aircraft_throttle(&game->aircraft, 1);
    }
    if (pressed(game, input, 's')) {
        aircraft_throttle(&game->aircraft, -1);
    }
    if (pressed(game, input, 'i')) {
        game->inverted_pitch = !game->inverted_pitch;
        game->invert_note = INVERT_NOTE_SECONDS;
    }
    if (pressed(game, input, 'c')) {
        game->cockpit_view = !game->cockpit_view;
    }

    aircraft_step(&game->aircraft, &controls, dt);
    camera_chase_step(&game->chase, game->aircraft.orientation, dt);
    if (game->invert_note > 0.0f) {
        game->invert_note -= dt;
    }
    if (touches_ground(game)) {
        game->state = GAME_CRASH;
    }
}

static void reset_view(Game *game)
{
    game->view_yaw = VIEW_YAW;
    game->view_pitch = VIEW_PITCH;
    game->view_distance = VIEW_DISTANCE;
    game->fly.position = v3_add(game->aircraft.position, game->chase.eye_offset);
    // the fly camera counts its yaw the other way round from the compass
    game->fly.yaw = -aircraft_heading(&game->aircraft);
    game->fly.pitch = 0.0f;
}

static void step_orbit(Game *game, const Input *input, float dt, int turn_left, int tilt_up)
{
    const float turn = ORBIT_RATE * dt;

    game->view_yaw += turn * (float)turn_left;
    game->view_pitch = clamped(game->view_pitch + turn * (float)tilt_up, VIEW_MIN_PITCH, VIEW_MAX_PITCH);
    // the unshifted plus key is what a keyboard actually sends
    if (input_key_is_down(input, '+') || input_key_is_down(input, '=')) {
        game->view_distance /= 1.0f + (ZOOM_RATE - 1.0f) * dt;
    }
    if (input_key_is_down(input, '-')) {
        game->view_distance *= 1.0f + (ZOOM_RATE - 1.0f) * dt;
    }
    game->view_distance = clamped(game->view_distance, VIEW_MIN_DISTANCE, VIEW_MAX_DISTANCE);
}

static void step_fly(Game *game, const Input *input, float dt, int turn_left, int tilt_up)
{
    const float speed = (input_shift_is_down(input) ? FREE_FAST_SPEED : FREE_SPEED) * dt;
    const Vec3 move = v3((float)(input_key_is_down(input, 'd') - input_key_is_down(input, 'a')) * speed,
                         (float)(input_key_is_down(input, 'e') - input_key_is_down(input, 'q')) * speed,
                         (float)(input_key_is_down(input, 'w') - input_key_is_down(input, 's')) * speed);

    camera_fly_step(&game->fly, move, LOOK_RATE * dt * (float)turn_left, LOOK_RATE * dt * (float)tilt_up);
}

static void step_pause(Game *game, const Input *input, float dt)
{
    const int turn_left = input_special_is_down(input, GLUT_KEY_LEFT) - input_special_is_down(input, GLUT_KEY_RIGHT);
    const int tilt_up = input_special_is_down(input, GLUT_KEY_UP) - input_special_is_down(input, GLUT_KEY_DOWN);

    // the free camera already uses Q and E for down and up, so only the orbit view quits on Q
    if (!game->free_camera && pressed(game, input, 'q')) {
        exit(EXIT_SUCCESS);
    }
    if (pressed(game, input, 'v')) {
        game->free_camera = !game->free_camera;
    }
    if (pressed(game, input, 'r')) {
        reset_view(game);
    }
    if (game->free_camera) {
        step_fly(game, input, dt, turn_left, tilt_up);
    } else {
        step_orbit(game, input, dt, turn_left, tilt_up);
    }
}

void game_step(void *context, const Input *input, float dt)
{
    Game *game = context;

    switch (game->state) {
    case GAME_FLIGHT:
        if (pressed(game, input, KEY_ESCAPE)) {
            game->state = GAME_PAUSE;
            game->free_camera = 0;
            reset_view(game);
        } else {
            step_flight(game, input, dt);
            game->time += dt;
        }
        break;
    case GAME_PAUSE:
        if (pressed(game, input, KEY_ESCAPE)) {
            game->state = GAME_FLIGHT;
        } else {
            step_pause(game, input, dt);
        }
        break;
    case GAME_CRASH:
        if (pressed(game, input, KEY_ENTER)) {
            start_flight(game);
        }
        game->time += dt;
        break;
    default:
        break;
    }

    game->previous = *input;
}

// the shadow map follows the sun ray through the jet down to the ground, so the jet and the terrain under it
// share one fit
static void aim_shadow(Game *game)
{
    const Vec3 sun = game->light.sun_direction;
    const Vec3 position = game->aircraft.position;
    const float altitude = position.y - terrain_height(position.x, position.z);

    game->renderer.shadow_center = v3_sub(position, v3_scale(sun, altitude / sun.y));
    game->renderer.shadow_radius = SHADOW_RADIUS;
    game->renderer.shadow_relief = SHADOW_RELIEF;
    game->renderer.shadow_reach = altitude;
}

// the pause always looks at the jet from outside, whatever view the flight was using
static int cockpit_camera(const Game *game)
{
    return game->cockpit_view && game->state != GAME_PAUSE;
}

static void place_camera(Game *game)
{
    const Aircraft *aircraft = &game->aircraft;

    if (game->state == GAME_PAUSE) {
        if (game->free_camera) {
            camera_fly(&game->camera, &game->fly);
        } else {
            const Vec3 center = v3_add(aircraft->position, quat_rotate(aircraft->orientation, game->jet_center));

            camera_orbit(&game->camera, center, game->view_yaw, game->view_pitch, game->view_distance);
        }
    } else if (cockpit_camera(game)) {
        camera_attached(&game->camera, aircraft->position, aircraft->orientation, game->cockpit_eye);
    } else {
        camera_chase(&game->camera, &game->chase, aircraft->position);
    }
}

static void fill_hud(const Game *game, HudState *state)
{
    state->speed = game->aircraft.speed;
    state->altitude = game->aircraft.position.y;
    state->heading = aircraft_heading(&game->aircraft) / VEC_DEGREES;
    state->throttle = game->aircraft.throttle;
    state->afterburner = game->aircraft.afterburner;
    state->message = NULL;
    state->hint = NULL;

    if (game->state == GAME_PAUSE) {
        state->message = "PAUSED";
        state->hint = game->free_camera ? "Esc fly on   WASD fly   Q/E down/up   arrows look   R reset   V orbit"
                                        : "Esc fly on   arrows orbit   +/- zoom   R reset   V free camera   Q quit";
    } else if (game->state == GAME_CRASH) {
        state->message = "Crashed";
        state->hint = "Enter restarts";
    } else if (game->invert_note > 0.0f) {
        state->message = game->inverted_pitch ? "Pitch inverted" : "Pitch normal";
    }
}

void game_render(void *context, int width, int height)
{
    Game *game = context;
    HudState state;

    game->jet->position = game->aircraft.position;
    game->jet->orientation = game->aircraft.orientation;
    // from inside the cockpit the jet's own hull would fill the view, but its shadow still belongs on the ground
    game->jet->hidden = cockpit_camera(game);

    place_camera(game);
    aim_shadow(game);
    renderer_draw(&game->renderer, &game->scene, &game->camera, &game->light, game->time, width, height);

    fill_hud(game, &state);
    hud_draw(&game->hud, &state, width, height);
}
