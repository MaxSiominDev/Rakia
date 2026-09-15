#include "check.h"
#include "engine/terrain.h"
#include "engine/timestep.h"
#include "engine/vecmath.h"
#include "game/game.h"

#include <string.h>

#define KEY_ESCAPE 27
#define KEY_ENTER 13

// the F-16 at its scale, which is all the ground contact needs from the model
#define JET_MIN v3(-4.7f, 0.0f, -7.5f)
#define JET_MAX v3(4.7f, 4.6f, 7.5f)

static Game game;
static Mesh shell;
static MeshGroup group;

static void hands_off(Input *input)
{
    memset(input, 0, sizeof *input);
}

static void steps(Input *input, int count)
{
    int step;

    for (step = 0; step < count; step++) {
        game_step(&game, input, TIMESTEP_DT);
    }
}

// a press only counts on the step it appears, so every key goes down after a step without it
static void tap(unsigned char key)
{
    Input input;

    hands_off(&input);
    steps(&input, 1);
    input_key(&input, key, 1);
    steps(&input, 1);
}

// the world without its models: the jet and everything bolted to it, which is all the game itself moves
static void fresh(void)
{
    memset(&game, 0, sizeof game);
    memset(&shell, 0, sizeof shell);
    memset(&group, 0, sizeof group);
    group.index_count = 3;
    shell.groups = &group;
    shell.group_count = 1;
    game.world.jet = scene_add(&game.scene, &shell, NULL);
    game.world.gear = scene_add(&game.scene, &shell, NULL);
    game.world.roundels = scene_add(&game.scene, &shell, NULL);
    game.world.rails[0] = scene_add(&game.scene, &shell, NULL);
    game.world.rails[1] = scene_add(&game.scene, &shell, NULL);
    game.world.jet_min = JET_MIN;
    game.world.jet_max = JET_MAX;
    // one missile, so a loadout carried through a crash has something to hang on a pylon
    game.world.props[0].entity = scene_add(&game.scene, &shell, NULL);
    game.world.props[0].kind = PROP_MISSILE;
    game.world.prop_count = 1;
    hangar_init(&game.hangar);
}

static void place(Vec3 position, float climb_degrees)
{
    aircraft_place(&game.aircraft, position, 0.0f, 250.0f, 0.7f);
    game.aircraft.orientation = quat_from_axis_angle(v3(1.0f, 0.0f, 0.0f), -climb_degrees * VEC_DEGREES);
    game.state = GAME_FLIGHT;
}

static void airborne(void)
{
    fresh();
    place(v3(0.0f, 600.0f, 0.0f), 0.0f);
    camera_chase_settle(&game.chase, game.aircraft.orientation);
}

static void on_apron(void)
{
    fresh();
    world_park(&game.world, &game.aircraft);
    game.state = GAME_HANGAR;
}

static float climb_sine(void)
{
    return quat_rotate(game.aircraft.orientation, v3(0.0f, 0.0f, 1.0f)).y;
}

static void test_hangar(void)
{
    Input input;
    float opened_at;
    float opened_pitch;

    on_apron();
    check_v3(game.aircraft.position, WORLD_APRON_X, WORLD_STAND, WORLD_PARK_Z, "the jet opens parked on the apron");

    tap('r');
    opened_at = game.view_distance;
    opened_pitch = game.view_pitch;
    hands_off(&input);
    input_special(&input, GLUT_KEY_UP, 1);
    input_key(&input, '-', 1);
    steps(&input, TIMESTEP_HZ);
    check(game.aircraft.position.y == WORLD_STAND, "the stick does nothing on the ground");
    check(game.view_pitch > opened_pitch, "but the arrows swing the hangar camera around");
    check(game.view_distance > opened_at, "and the minus key backs it off");

    tap('r');
    check_close(game.view_distance, opened_at, 1e-5f, "r puts the hangar view back where it started");
    check_close(game.view_pitch, opened_pitch, 1e-5f, "at the angle it opened on too");

    tap(KEY_ESCAPE);
    check(game.state == GAME_PAUSE && game.resume == GAME_HANGAR, "escape pauses the hangar");
    tap(KEY_ESCAPE);
    check(game.state == GAME_HANGAR, "and escape goes back to it");

    tap(KEY_ENTER);
    check(game.state == GAME_TAKEOFF, "enter starts the takeoff, loaded or not");
}

static void test_tint_release(void)
{
    on_apron();
    game.world.props[0].entity->highlight = 0.6f;
    tap(KEY_ENTER);
    check(game.state == GAME_TAKEOFF, "the takeoff starts");
    check(game.world.props[0].entity->highlight == 0.0f, "and the cursor lets go of what it was holding");
}

static void test_takeoff_handover(void)
{
    Input input;
    int step;

    on_apron();
    tap(KEY_ENTER);
    hands_off(&input);
    for (step = 0; step < 40 * TIMESTEP_HZ && game.state == GAME_TAKEOFF; step++) {
        steps(&input, 1);
    }
    check(game.state == GAME_FLIGHT, "the script flies the jet all the way into the player's hands");
    check(game.aircraft.position.y > WORLD_STAND + 100.0f, "well above the airbase");
    check(game.aircraft.speed > 100.0f, "and fast enough to stay there");
}

static void test_pause(void)
{
    Vec3 parked;
    Input input;

    airborne();
    tap(KEY_ESCAPE);
    check(game.state == GAME_PAUSE && game.resume == GAME_FLIGHT, "escape pauses the flight");

    parked = game.aircraft.position;
    hands_off(&input);
    input_special(&input, GLUT_KEY_UP, 1);
    steps(&input, TIMESTEP_HZ);
    check(game.aircraft.position.x == parked.x && game.aircraft.position.y == parked.y &&
              game.aircraft.position.z == parked.z,
          "a paused jet does not move");

    tap(KEY_ESCAPE);
    check(game.state == GAME_FLIGHT, "and escape lets it fly on");
    steps(&input, TIMESTEP_HZ);
    check(game.aircraft.position.z > parked.z, "which it does");
}

static void test_crash(void)
{
    Input input;

    hands_off(&input);
    airborne();
    place(v3(0.0f, terrain_height(0.0f, 0.0f) + 40.0f, 0.0f), -30.0f);
    steps(&input, TIMESTEP_HZ);
    check(game.state == GAME_CRASH, "flying into the ground crashes the jet");

    airborne();
    place(v3(5000.0f, TERRAIN_SEA_LEVEL - 1.0f, 0.0f), 0.0f);
    check(terrain_height(5000.0f, 0.0f) < TERRAIN_SEA_LEVEL, "the land west of the coast lies under the sea");
    steps(&input, 1);
    check(game.state == GAME_CRASH, "and so does touching the sea, high above the sea floor");

    airborne();
    place(v3(0.0f, terrain_height(0.0f, 0.0f) + 3.0f, 0.0f), 0.0f);
    steps(&input, 1);
    check(game.state == GAME_FLIGHT, "three metres of air under the belly is still flight");
    // a wing tip hangs lower than the belly once the jet is over, which is why the whole box is walked
    game.aircraft.orientation = quat_from_axis_angle(v3(0.0f, 0.0f, 1.0f), 70.0f * VEC_DEGREES);
    steps(&input, 1);
    check(game.state == GAME_CRASH, "and the same three metres with the wings banked over is not");
}

static void test_crash_return(void)
{
    Input input;

    hands_off(&input);
    airborne();
    game.hangar.mounted[1] = 0;
    place(v3(0.0f, terrain_height(0.0f, 0.0f) - 1.0f, 0.0f), 0.0f);
    steps(&input, 1);
    check(game.state == GAME_CRASH, "the jet is on the ground");

    steps(&input, 2 * TIMESTEP_HZ);
    check(game.state == GAME_CRASH, "the wreck stays on screen for a moment");
    // the wait is counted down a dt at a time, so it runs out a step or two past the three seconds
    steps(&input, TIMESTEP_HZ + 2);
    check(game.state == GAME_HANGAR, "and after three seconds the hangar opens again");
    check_v3(game.aircraft.position, WORLD_APRON_X, WORLD_STAND, WORLD_PARK_Z, "with the jet back on its spot");
    check(game.view_distance > 0.0f && game.view_pitch > 0.0f, "and the camera looking at it from the apron");
    check(hangar_missiles(&game.hangar) == 1, "and the loadout it took off with");
}

static void test_keys_outside_flight(void)
{
    Input input;

    hands_off(&input);
    airborne();
    place(v3(0.0f, terrain_height(0.0f, 0.0f) - 1.0f, 0.0f), 0.0f);
    steps(&input, 1);
    tap(KEY_ESCAPE);
    check(game.state == GAME_CRASH, "a crashed jet ignores the pause key");

    input_special(&input, GLUT_KEY_UP, 1);
    input_key(&input, 'w', 1);
    steps(&input, TIMESTEP_HZ);
    check(game.aircraft.throttle == 0.7f && climb_sine() == 0.0f, "and the stick and the throttle with it");

    airborne();
    tap(KEY_ESCAPE);
    tap('w');
    check(game.state == GAME_PAUSE && game.aircraft.throttle == 0.7f, "a paused jet ignores the throttle");

    tap(KEY_ENTER);
    check(game.state == GAME_PAUSE, "and enter, which only takes off from the hangar");
}

static void test_pause_views(void)
{
    const float start_yaw = 35.0f * VEC_DEGREES;
    const float start_distance = 22.0f;
    Input input;
    Vec3 parked;

    airborne();
    tap(KEY_ESCAPE);
    hands_off(&input);
    input_special(&input, GLUT_KEY_LEFT, 1);
    input_key(&input, '+', 1);
    steps(&input, TIMESTEP_HZ);
    check(game.view_yaw > start_yaw + 0.5f, "the arrows swing the paused camera around the jet");
    check(game.view_distance < start_distance, "and the plus key pulls it closer");

    tap('r');
    check_close(game.view_yaw, start_yaw, 1e-5f, "r puts the view back where the pause opened it");
    check_close(game.view_distance, start_distance, 1e-5f, "at that distance too");

    tap('v');
    check(game.free_camera, "v hands the pause over to the free camera");
    parked = game.fly.position;
    hands_off(&input);
    input_key(&input, 'w', 1);
    steps(&input, TIMESTEP_HZ);
    check(game.fly.position.z > parked.z + 10.0f, "which flies north on its own");
}

static void test_cockpit_view(void)
{
    airborne();
    tap('c');
    check(game.cockpit_view, "c moves the view into the cockpit");

    tap(KEY_ESCAPE);
    tap('c');
    check(game.state == GAME_PAUSE && game.cockpit_view, "the pause leaves the view key alone");

    tap(KEY_ESCAPE);
    tap('c');
    check(game.state == GAME_FLIGHT && !game.cockpit_view, "and in flight it moves back out");
}

static void test_throttle_and_inversion(void)
{
    Input input;
    float climbing;

    airborne();
    hands_off(&input);
    input_key(&input, 'w', 1);
    steps(&input, TIMESTEP_HZ);
    check_close(game.aircraft.throttle, 0.8f, 1e-5f, "a held throttle key moves one notch, not one per step");

    airborne();
    hands_off(&input);
    input_special(&input, GLUT_KEY_UP, 1);
    steps(&input, TIMESTEP_HZ);
    climbing = climb_sine();
    check(climbing > 0.0f, "the up key climbs");

    airborne();
    tap('i');
    check(game.inverted_pitch && game.invert_note > 0.0f, "i inverts the pitch keys and says so");
    hands_off(&input);
    input_special(&input, GLUT_KEY_UP, 1);
    steps(&input, TIMESTEP_HZ);
    check_close(climb_sine(), -climbing, 1e-3f, "and then the same key dives by the same amount");
}

void test_game_main(void)
{
    test_hangar();
    test_tint_release();
    test_takeoff_handover();
    test_pause();
    test_crash();
    test_crash_return();
    test_keys_outside_flight();
    test_pause_views();
    test_cockpit_view();
    test_throttle_and_inversion();
}
