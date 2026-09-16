#include "game/game.h"

#include "engine/assets.h"
#include "engine/terrain.h"
#include "game/hud.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define KEY_ESCAPE 27
#define KEY_ENTER 13
#define KEY_TAB 9
#define KEY_SPACE 32

#define SKY_PANORAMA "raw/sky/belfast_sunset_puresky/belfast_sunset_puresky_2k.hdr"

// the sun is ten degrees up, so the jet's shadow lands more than two kilometres ahead of it: the map is fitted
// around the ground the sun ray through the jet reaches, and reaches back up to the jet itself
#define SHADOW_RADIUS 900.0f
// room for the dunes between the jet and its shadow
#define SHADOW_RELIEF 200.0f
// the hangar wants none of that reach: a tight box around the apron keeps the shelters and the cart crisp
#define APRON_SHADOW_RADIUS 60.0f
#define APRON_SHADOW_RELIEF 12.0f
#define APRON_SHADOW_REACH 12.0f

#define VIEW_YAW (35.0f * VEC_DEGREES)
#define VIEW_PITCH (8.0f * VEC_DEGREES)
#define VIEW_DISTANCE 22.0f
// the hangar looks at the jet from further back and higher up, so the cart and the shelters fit the frame
#define HANGAR_YAW (45.0f * VEC_DEGREES)
#define HANGAR_PITCH (18.0f * VEC_DEGREES)
#define HANGAR_DISTANCE 27.0f
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
#define CRASH_SECONDS 3.0f
// a fresh layout and a fresh effects stream every mission, from a counter that never repeats a seed
#define TARGETS_SEED 5303u
#define EFFECTS_SEED 8161u
#define CRASH_FIRE_SIZE 26.0f

// below this and sinking, or the terrain ahead climbing into the flight path within this long
#define PULL_UP_HEIGHT 60.0f
#define PULL_UP_LOOKAHEAD 3.0f
#define PULL_UP_SAMPLES 4

static GameState shown_state(const Game *game)
{
    return game->state == GAME_PAUSE ? game->resume : game->state;
}

static int in_hangar(const Game *game)
{
    return shown_state(game) == GAME_HANGAR;
}

static void reset_view(Game *game)
{
    const int hangar = in_hangar(game);

    game->view_yaw = hangar ? HANGAR_YAW : VIEW_YAW;
    game->view_pitch = hangar ? HANGAR_PITCH : VIEW_PITCH;
    game->view_distance = hangar ? HANGAR_DISTANCE : VIEW_DISTANCE;
    game->fly.position = v3_add(game->aircraft.position, game->chase.eye_offset);
    // the fly camera counts its yaw the other way round from the compass
    game->fly.yaw = -aircraft_heading(&game->aircraft);
    game->fly.pitch = 0.0f;
}

// the pause and the hangar always look at the jet from outside, whatever view the flight was using
static int cockpit_camera(const Game *game)
{
    return game->cockpit_view && game->state != GAME_PAUSE && !in_hangar(game);
}

static Vec3 orbit_target(const Game *game)
{
    if (in_hangar(game)) {
        // the parking spot rather than the jet, so dragging the jet does not drag the camera after it
        return v3_add(v3(WORLD_APRON_X, WORLD_STAND, WORLD_PARK_Z), game->world.jet_center);
    }

    return v3_add(game->aircraft.position, quat_rotate(game->aircraft.orientation, game->world.jet_center));
}

static void place_camera(Game *game)
{
    if (game->state == GAME_PAUSE && game->free_camera) {
        camera_fly(&game->camera, &game->fly);
    } else if (in_hangar(game) || game->state == GAME_PAUSE) {
        camera_orbit(&game->camera, orbit_target(game), game->view_yaw, game->view_pitch, game->view_distance);
    } else if (cockpit_camera(game)) {
        camera_attached(&game->camera, game->aircraft.position, game->aircraft.orientation,
                        game->world.cockpit_eye);
    } else {
        camera_chase(&game->camera, &game->chase, game->aircraft.position);
    }
}

static void enter_hangar(Game *game)
{
    world_park(&game->world, &game->aircraft);
    game->state = GAME_HANGAR;
    game->cockpit_view = 0;
    reset_view(game);
}

// what a crash and a mission complete both reset; a rearm mid-mission keeps the targets and the counters
static void fresh_mission(Game *game)
{
    targets_place(&game->targets, TARGETS_SEED + game->mission);
    effects_reset(&game->effects, EFFECTS_SEED + game->mission);
    weapons_reset(&game->weapons);
    game->mission++;
    game->mission_time = 0.0f;
    game->designated = -1;
    enter_hangar(game);
}

static void open_hangar(Game *game)
{
    fresh_mission(game);
    hangar_restock_cart(&game->hangar, &game->world);
}

// a crash keeps the sortie's loadout, so the fired missiles go back onto their own pylons, not the cart
static void crash_return(Game *game)
{
    fresh_mission(game);
    hangar_restock_pylons(&game->hangar, &game->world);
}

// the world clock keeps running through the hangar, so only time actually spent in flight counts
static void end_sortie(Game *game)
{
    game->mission_time += game->time - game->mission_start;
}

static int out_of_missiles(const Game *game)
{
    return hangar_missiles(&game->hangar) == 0 && weapons_flying(&game->weapons) == 0;
}

// Enter on the offer: the hangar opens at once, but the mission keeps its targets and its counters
static void rearm(Game *game)
{
    end_sortie(game);
    enter_hangar(game);
    hangar_restock_cart(&game->hangar, &game->world);
}

static void start_takeoff(Game *game)
{
    hangar_release(&game->hangar, &game->world);
    hangar_takeoff(&game->hangar);
    // nothing in the hangar turns the jet, so it still faces the way it was parked
    aircraft_place(&game->aircraft, game->world.jet->position, WORLD_PARK_HEADING, 0.0f, 0.0f);
    takeoff_begin(&game->takeoff, &game->aircraft);
    camera_chase_settle(&game->chase, game->aircraft.orientation);
    game->state = GAME_TAKEOFF;
}

int game_init(void *context)
{
    Game *game = context;
    char panorama[ASSETS_PATH_MAX];

    if (assets_path(panorama, sizeof panorama, SKY_PANORAMA) != 0) {
        fprintf(stderr, "asset path is too long: %s\n", SKY_PANORAMA);
        return -1;
    }
    if (renderer_init(&game->renderer, panorama) != 0 ||
        world_init(&game->world, &game->scene, &game->aircraft) != 0 || text_init(&game->hud) != 0 ||
        particles_init(&game->particles) != 0 ||
        targets_init(&game->targets, &game->world, &game->scene) != 0 ||
        effects_init(&game->effects, &game->particles, &game->scene) != 0) {
        return -1;
    }

    game->light = light_golden_hour();
    game->camera.fov_y_radians = 45.0f * VEC_DEGREES;
    game->camera.near_plane = 1.0f;
    // the chunk ring reaches past the fog, and a nearer plane would fight for depth over the apron
    game->camera.far_plane = 4.0f * TERRAIN_VIEW_DISTANCE;
    hangar_init(&game->hangar);
    open_hangar(game);
    // the first tick picks with the cursor before anything has been drawn, so the ray needs a camera by then
    place_camera(game);

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
        const Vec3 local = v3(corner & 1 ? game->world.jet_max.x : game->world.jet_min.x,
                              corner & 2 ? game->world.jet_max.y : game->world.jet_min.y,
                              corner & 4 ? game->world.jet_max.z : game->world.jet_min.z);
        const Vec3 world = m4_transform_point(model, local);

        if (world.y <= fmaxf(terrain_height(world.x, world.z), TERRAIN_SEA_LEVEL)) {
            return 1;
        }
    }

    return 0;
}

// Tab moves to the next live target; whichever one is designated, a destroyed one gives way to the nearest
static void update_designated(Game *game, const Input *input)
{
    if (game->designated < 0 || !game->targets.list[game->designated].alive) {
        game->designated = targets_nearest(&game->targets, game->aircraft.position);
    }
    if (pressed(game, input, KEY_TAB) && game->designated >= 0) {
        game->designated = targets_next(&game->targets, game->designated);
    }
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

    update_designated(game, input);
    if (pressed(game, input, KEY_SPACE)) {
        weapons_launch(&game->weapons, &game->hangar, &game->world, &game->aircraft,
                      weapons_locked(&game->targets, &game->aircraft, game->designated));
    }

    if (touches_ground(game)) {
        effects_explosion(&game->effects, game->aircraft.position, CRASH_FIRE_SIZE);
        game->state = GAME_CRASH;
        game->crash_wait = CRASH_SECONDS;
    } else if (targets_alive(&game->targets) == 0) {
        end_sortie(game);
        game->state = GAME_COMPLETE;
    } else if (pressed(game, input, KEY_ENTER) && out_of_missiles(game)) {
        rearm(game);
    }
}

static void step_takeoff(Game *game, float dt)
{
    takeoff_step(&game->takeoff, &game->aircraft, dt);
    camera_chase_step(&game->chase, game->aircraft.orientation, dt);
    if (game->takeoff.phase == TAKEOFF_DONE) {
        game->state = GAME_FLIGHT;
        game->mission_start = game->time;
    }
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

static Ray cursor_ray(const Game *game, const Input *input)
{
    const Mat4 view_projection =
        m4_multiply(camera_projection(&game->camera, input->window_width, input->window_height),
                    camera_view(&game->camera));

    return picking_ray(view_projection, (float)input->mouse_x, (float)input->mouse_y,
                       (float)input->window_width, (float)input->window_height);
}

static void step_hangar(Game *game, const Input *input, float dt)
{
    const int turn_left = (input_special_is_down(input, GLUT_KEY_LEFT) || input_key_is_down(input, 'a')) -
                          (input_special_is_down(input, GLUT_KEY_RIGHT) || input_key_is_down(input, 'd'));
    const int tilt_up = (input_special_is_down(input, GLUT_KEY_UP) || input_key_is_down(input, 'w')) -
                        (input_special_is_down(input, GLUT_KEY_DOWN) || input_key_is_down(input, 's'));

    if (pressed(game, input, KEY_ENTER)) {
        start_takeoff(game);
        return;
    }
    if (pressed(game, input, 'r')) {
        reset_view(game);
    }
    step_orbit(game, input, dt, turn_left, tilt_up);
    hangar_step(&game->hangar, &game->world, cursor_ray(game, input), input_button_is_down(input, GLUT_LEFT_BUTTON));
}

static void pause_game(Game *game)
{
    game->resume = game->state;
    game->state = GAME_PAUSE;
    game->free_camera = 0;
    reset_view(game);
}

void game_step(void *context, const Input *input, float dt)
{
    Game *game = context;

    // the world clock stops with the pause, so the sea and every explosion hold still with it
    if (game->state != GAME_PAUSE) {
        game->time += dt;
        particles_step(&game->particles, dt);
        effects_step(&game->effects, dt);
        targets_step(&game->targets, dt);
        weapons_step(&game->weapons, &game->targets, &game->effects, dt);
    }

    switch (game->state) {
    case GAME_HANGAR:
        if (pressed(game, input, KEY_ESCAPE)) {
            pause_game(game);
        } else {
            step_hangar(game, input, dt);
        }
        break;
    case GAME_TAKEOFF:
        if (pressed(game, input, KEY_ESCAPE)) {
            pause_game(game);
        } else {
            step_takeoff(game, dt);
        }
        break;
    case GAME_FLIGHT:
        if (pressed(game, input, KEY_ESCAPE)) {
            pause_game(game);
        } else {
            step_flight(game, input, dt);
        }
        break;
    case GAME_PAUSE:
        if (pressed(game, input, KEY_ESCAPE)) {
            game->state = game->resume;
        } else {
            step_pause(game, input, dt);
        }
        break;
    case GAME_CRASH:
        game->crash_wait -= dt;
        if (game->crash_wait <= 0.0f) {
            crash_return(game);
        }
        break;
    case GAME_COMPLETE:
        if (pressed(game, input, KEY_ENTER)) {
            open_hangar(game);
        }
        break;
    default:
        break;
    }

    game->previous = *input;
}

// outside the hangar the shadow map follows the sun ray through the jet down to the ground, so the jet and the
// terrain under it share one fit
static void aim_shadow(Game *game)
{
    const Vec3 sun = game->light.sun_direction;
    const Vec3 position = game->aircraft.position;
    float altitude;

    if (in_hangar(game)) {
        game->renderer.shadow_center = v3(WORLD_APRON_X, 0.0f, WORLD_PARK_Z);
        game->renderer.shadow_radius = APRON_SHADOW_RADIUS;
        game->renderer.shadow_relief = APRON_SHADOW_RELIEF;
        game->renderer.shadow_reach = APRON_SHADOW_REACH;
        return;
    }
    altitude = position.y - terrain_height(position.x, position.z);
    game->renderer.shadow_center = v3_sub(position, v3_scale(sun, altitude / sun.y));
    game->renderer.shadow_radius = SHADOW_RADIUS;
    game->renderer.shadow_relief = SHADOW_RELIEF;
    game->renderer.shadow_reach = altitude;
}

static float canopy_shut(const Game *game)
{
    const GameState shown = shown_state(game);

    if (shown == GAME_HANGAR) {
        return 0.0f;
    }

    return shown == GAME_TAKEOFF ? game->takeoff.canopy : 1.0f;
}

static int gear_is_down(const Game *game)
{
    const GameState shown = shown_state(game);

    return shown == GAME_HANGAR || (shown == GAME_TAKEOFF && !game->takeoff.gear_up);
}

static int crashed(const Game *game)
{
    return shown_state(game) == GAME_CRASH;
}

// the angle a world point sits at around the screen center: 0 up, growing clockwise
static float screen_bearing(const Game *game, Vec3 point)
{
    const Vec3 to_point = v3_normalize(v3_sub(point, game->aircraft.position));
    const Vec3 right = quat_rotate(game->aircraft.orientation, v3(1.0f, 0.0f, 0.0f));
    const Vec3 up = quat_rotate(game->aircraft.orientation, v3(0.0f, 1.0f, 0.0f));

    return atan2f(v3_dot(to_point, right), v3_dot(to_point, up));
}

// the screen-space rectangle the box's eight corners project into; 0 when every corner is behind the eye
static int project_box(const Game *game, Hitbox box, int width, int height, float *x0, float *y0, float *x1,
                       float *y1)
{
    const Mat4 view_projection =
        m4_multiply(camera_projection(&game->camera, width, height), camera_view(&game->camera));
    const Mat4 model = m4_multiply(m4_translate(box.position), m4_rotate(box.yaw, v3(0.0f, 1.0f, 0.0f)));
    int any = 0;
    int corner;

    for (corner = 0; corner < 8; corner++) {
        const Vec3 local = v3(corner & 1 ? box.max.x : box.min.x, corner & 2 ? box.max.y : box.min.y,
                              corner & 4 ? box.max.z : box.min.z);
        const Vec3 world = m4_transform_point(model, local);
        float x;
        float y;

        if (!picking_screen(view_projection, world, (float)width, (float)height, &x, &y)) {
            continue;
        }
        if (!any) {
            *x0 = *x1 = x;
            *y0 = *y1 = y;
        } else {
            *x0 = fminf(*x0, x);
            *x1 = fmaxf(*x1, x);
            *y0 = fminf(*y0, y);
            *y1 = fmaxf(*y1, y);
        }
        any = 1;
    }

    return any;
}

static int pull_up_needed(const Game *game)
{
    const Vec3 position = game->aircraft.position;
    const Vec3 velocity =
        v3_scale(quat_rotate(game->aircraft.orientation, v3(0.0f, 0.0f, 1.0f)), game->aircraft.speed);
    const float above_ground = position.y - fmaxf(terrain_height(position.x, position.z), TERRAIN_SEA_LEVEL);
    int sample;

    if (above_ground < PULL_UP_HEIGHT && velocity.y < 0.0f) {
        return 1;
    }
    for (sample = 1; sample <= PULL_UP_SAMPLES; sample++) {
        const float seconds = PULL_UP_LOOKAHEAD * (float)sample / (float)PULL_UP_SAMPLES;
        const Vec3 ahead = v3_add(position, v3_scale(velocity, seconds));

        if (fmaxf(terrain_height(ahead.x, ahead.z), TERRAIN_SEA_LEVEL) > ahead.y) {
            return 1;
        }
    }

    return 0;
}

static void fill_hud(const Game *game, HudState *state, int width, int height, char *summary, size_t summary_size)
{
    state->in_flight = !in_hangar(game);
    state->speed = game->aircraft.speed;
    state->altitude = game->aircraft.position.y;
    state->heading = aircraft_heading(&game->aircraft) / VEC_DEGREES;
    state->throttle = game->aircraft.throttle;
    state->afterburner = game->aircraft.afterburner;
    state->missiles = hangar_missiles(&game->hangar);
    state->pylons = WORLD_PYLONS;
    state->targets_destroyed = TARGETS_COUNT - targets_alive(&game->targets);
    state->targets_total = TARGETS_COUNT;
    state->has_target = game->state == GAME_FLIGHT && game->designated >= 0;
    state->locked = 0;
    state->pull_up = game->state == GAME_FLIGHT && pull_up_needed(game);
    state->message = NULL;
    state->hint = NULL;

    if (state->has_target) {
        const Target *target = &game->targets.list[game->designated];
        const Vec3 center = target_center(target);
        const int on_target = weapons_locked(&game->targets, &game->aircraft, game->designated) >= 0;

        state->target_bearing = screen_bearing(game, center);
        state->target_range = v3_length(v3_sub(center, game->aircraft.position));
        state->locked = on_target && project_box(game, target_hitbox(target), width, height, &state->lock_x0,
                                                  &state->lock_y0, &state->lock_x1, &state->lock_y1);
    }

    if (game->state == GAME_PAUSE) {
        state->message = "PAUSED";
        state->hint = game->free_camera ? "Esc back   WASD fly   Q/E down/up   arrows look   R reset   V orbit"
                                        : "Esc back   arrows orbit   +/- zoom   R reset   V free camera   Q quit";
    } else if (game->state == GAME_CRASH) {
        state->message = "Crashed";
        state->hint = "Back to the hangar";
    } else if (game->state == GAME_COMPLETE) {
        snprintf(summary, summary_size, "Time %.0f s   Launched %d   Hits %d", game->mission_time,
                game->weapons.launched, game->weapons.hits);
        state->message = "Mission complete";
        state->hint = summary;
    } else if (game->state == GAME_HANGAR) {
        state->hint = state->missiles > 0 ? "Drag the missiles onto the pylons   Enter takes off"
                                          : "No missiles mounted   drag them onto the pylons   Enter takes off anyway";
    } else if (game->state == GAME_FLIGHT && out_of_missiles(game)) {
        state->message = "No missiles left";
        state->hint = "Enter returns to base";
    } else if (game->invert_note > 0.0f) {
        state->message = game->inverted_pitch ? "Pitch inverted" : "Pitch normal";
    }
}

void game_render(void *context, int width, int height)
{
    Game *game = context;
    HudState state;
    char summary[64];

    if (!in_hangar(game)) {
        game->world.jet->position = game->aircraft.position;
        game->world.jet->orientation = game->aircraft.orientation;
    }
    world_canopy(&game->world, canopy_shut(game));
    world_follow(&game->world);
    hangar_carry(&game->hangar, &game->world);
    game->world.gear->hidden = !gear_is_down(game) || crashed(game);
    // hidden in the cockpit view since the hull would block the camera, but it still casts a shadow
    // the roundels and both rail missiles are bolted to the jet, so they hide with it once it has crashed
    game->world.jet->hidden = cockpit_camera(game) || crashed(game);
    game->world.roundels->hidden = crashed(game);
    game->world.rails[0]->hidden = crashed(game);
    game->world.rails[1]->hidden = crashed(game);

    place_camera(game);
    aim_shadow(game);
    renderer_draw(&game->renderer, &game->scene, &game->particles, &game->camera, &game->light, game->time, width,
                  height);

    fill_hud(game, &state, width, height, summary, sizeof summary);
    hud_draw(&game->hud, &state, width, height);
}
