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

static void place(Vec3 position, float climb_degrees)
{
    aircraft_place(&game.aircraft, position, 0.0f, 250.0f, 0.7f);
    game.aircraft.orientation = quat_from_axis_angle(v3(1.0f, 0.0f, 0.0f), -climb_degrees * VEC_DEGREES);
    game.state = GAME_FLIGHT;
}

static void airborne(void)
{
    memset(&game, 0, sizeof game);
    game.jet_min = JET_MIN;
    game.jet_max = JET_MAX;
    place(v3(0.0f, 600.0f, 0.0f), 0.0f);
    camera_chase_settle(&game.chase, game.aircraft.orientation);
}

static float climb_sine(void)
{
    return quat_rotate(game.aircraft.orientation, v3(0.0f, 0.0f, 1.0f)).y;
}

static void test_pause(void)
{
    Vec3 parked;
    Input input;

    airborne();
    tap(KEY_ESCAPE);
    check(game.state == GAME_PAUSE, "escape pauses the flight");

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

static void test_restart(void)
{
    const float start_x = 0.0f;
    const float start_z = -3500.0f;
    Input input;

    hands_off(&input);
    airborne();
    place(v3(0.0f, terrain_height(0.0f, 0.0f) - 1.0f, 0.0f), 0.0f);
    steps(&input, 1);
    check(game.state == GAME_CRASH, "the jet is on the ground");

    tap(KEY_ENTER);
    check(game.state == GAME_FLIGHT, "enter starts the flight again");
    check_close(game.aircraft.position.z, start_z, 1.0f, "from the start pose south of the base");
    check_close(game.aircraft.position.y, terrain_height(start_x, start_z) + 150.0f, 1.0f,
                "a hundred and fifty metres over the ground");
    check_close(game.aircraft.speed, 250.0f, 1.0f, "at the cruise speed");
    check_close(game.aircraft.throttle, 0.7f, 1e-5f, "and seventy percent throttle");
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
    check(game.state == GAME_PAUSE, "and enter, which only restarts a crashed one");
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
    test_pause();
    test_crash();
    test_restart();
    test_keys_outside_flight();
    test_pause_views();
    test_cockpit_view();
    test_throttle_and_inversion();
}
