#include "check.h"
#include "engine/timestep.h"
#include "engine/vecmath.h"
#include "game/takeoff.h"
#include "game/world.h"

#include <math.h>

// the whole sequence has to be over well before the player gets bored of watching it
#define TIME_LIMIT 30.0f
// a jet dragged to the far side of the apron has further to taxi, but not forever
#define SWEEP_LIMIT 60.0f

static Aircraft jet;
static Takeoff script;

static void park_at(Vec3 position, float heading)
{
    aircraft_place(&jet, position, heading, 0.0f, 0.0f);
    takeoff_begin(&script, &jet);
}

static int fly_script(void)
{
    const int limit = (int)(SWEEP_LIMIT * TIMESTEP_HZ);
    int steps = 0;

    while (script.phase != TAKEOFF_DONE && steps < limit) {
        takeoff_step(&script, &jet, TIMESTEP_DT);
        steps++;
    }

    return steps;
}

static void test_sequence(void)
{
    const int limit = (int)(TIME_LIMIT * TIMESTEP_HZ);
    TakeoffPhase phase;
    float lineup_x = 0.0f;
    float lineup_heading = 0.0f;
    float rotate_speed = 0.0f;
    int sank = 0;
    int left_the_runway = 0;
    int steps = 0;

    park_at(v3(WORLD_APRON_X, WORLD_STAND, WORLD_PARK_Z), WORLD_PARK_HEADING);
    phase = script.phase;
    while (script.phase != TAKEOFF_DONE && steps < limit) {
        takeoff_step(&script, &jet, TIMESTEP_DT);
        steps++;
        if (phase == TAKEOFF_TAXI && script.phase != phase) {
            lineup_x = jet.position.x;
            lineup_heading = script.heading;
        }
        if (phase == TAKEOFF_ROLL && script.phase != phase) {
            rotate_speed = jet.speed;
        }
        phase = script.phase;
        sank = sank || jet.position.y < WORLD_STAND - 0.001f;
        left_the_runway = left_the_runway ||
                          (script.phase != TAKEOFF_TAXI &&
                           fabsf(jet.position.x - WORLD_RUNWAY_X) > WORLD_RUNWAY_WIDTH * 0.5f);
    }

    check(script.phase == TAKEOFF_DONE, "the script gets the jet into the air");
    check((float)steps * TIMESTEP_DT < TIME_LIMIT, "inside half a minute");
    check(fabsf(lineup_x - WORLD_RUNWAY_X) < 6.0f, "the roll starts on the runway centerline");
    check(fabsf(lineup_heading - WORLD_RUNWAY_HEADING) < 5.0f * VEC_DEGREES, "with the nose down the runway");
    check_close(rotate_speed, 80.0f, 0.5f, "the nose comes up at eighty metres per second");
    check(!sank, "and the wheels stay on the paving until then");
    check(!left_the_runway, "the roll keeps the jet between the runway edges");
    check(jet.position.y - WORLD_STAND >= 150.0f, "the player takes over a hundred and fifty metres up");
    check(script.gear_up, "with the gear away");
    check_close(script.canopy, 1.0f, 1e-6f, "and the canopy shut");
    check(jet.position.z > WORLD_TAXI_Z, "north of the taxiway, which is the way the runway points");
}

static void test_canopy(void)
{
    int step;

    park_at(v3(WORLD_APRON_X, WORLD_STAND, WORLD_PARK_Z), WORLD_PARK_HEADING);
    check(script.canopy == 0.0f, "the canopy starts wide open");

    for (step = 0; step < 3 * TIMESTEP_HZ; step++) {
        takeoff_step(&script, &jet, TIMESTEP_DT);
    }
    check_close(script.canopy, 1.0f, 1e-6f, "and is shut a few seconds later");
    check(script.phase == TAKEOFF_TAXI, "while the jet is still taxiing");
    check(!script.gear_up, "with the gear still down");
}

// a start that leaves a waypoint inside the jet's own turning circle used to send it round that waypoint for
// good, and the player reaches those spots by dragging the jet across the apron
static void test_start_spots(void)
{
    int hung = 0;
    int missed = 0;
    int x;
    int z;

    for (x = -20; x <= 260; x += 20) {
        for (z = -1260; z <= -1040; z += 55) {
            park_at(v3((float)x, WORLD_STAND, (float)z), WORLD_PARK_HEADING);
            if (fly_script() >= (int)(SWEEP_LIMIT * TIMESTEP_HZ)) {
                hung++;
                continue;
            }
            missed += fabsf(jet.position.x - WORLD_RUNWAY_X) > WORLD_RUNWAY_WIDTH;
        }
    }
    check(hung == 0, "the jet taxis out and takes off from anywhere it can be dragged to");
    check(missed == 0, "and uses the runway to do it every time");
}

void test_takeoff_main(void)
{
    test_sequence();
    test_canopy();
    test_start_spots();
}
