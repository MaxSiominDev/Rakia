#include "check.h"
#include "engine/timestep.h"
#include "engine/vecmath.h"
#include "game/aircraft.h"

#include <math.h>

#define CRUISE_ALTITUDE 1000.0f
#define CRUISE_SPEED 250.0f

static const AircraftControls hands_off = {0.0f, 0.0f, 0.0f, 0};

static void fly(Aircraft *aircraft, const AircraftControls *controls, float seconds)
{
    int step;

    for (step = 0; step < (int)(seconds * TIMESTEP_HZ); step++) {
        aircraft_step(aircraft, controls, TIMESTEP_DT);
    }
}

static void cruise(Aircraft *aircraft, float throttle)
{
    aircraft_place(aircraft, v3(0.0f, CRUISE_ALTITUDE, 0.0f), 0.0f, CRUISE_SPEED, throttle);
}

// a positive turn about the left wing drops the nose, so a climb takes the negative angle
static void aim(Aircraft *aircraft, float climb_degrees)
{
    aircraft->orientation = quat_from_axis_angle(v3(1.0f, 0.0f, 0.0f), -climb_degrees * VEC_DEGREES);
}

static float climb_angle(const Aircraft *aircraft)
{
    return asinf(quat_rotate(aircraft->orientation, v3(0.0f, 0.0f, 1.0f)).y) / VEC_DEGREES;
}

static void test_trim_speeds(void)
{
    Aircraft aircraft;

    cruise(&aircraft, 0.7f);
    fly(&aircraft, &hands_off, 90.0f);
    check_close(aircraft.speed, CRUISE_SPEED, 5.0f, "seventy percent holds the cruise speed");

    cruise(&aircraft, 1.0f);
    fly(&aircraft, &hands_off, 90.0f);
    check_close(aircraft.speed, 320.0f, 5.0f, "full throttle settles at 320 m/s");

    cruise(&aircraft, 0.0f);
    fly(&aircraft, &hands_off, 90.0f);
    check(aircraft.speed < 100.0f, "idle throttle falls to the edge of the stall");
}

static void test_afterburner(void)
{
    const AircraftControls burner = {0.0f, 0.0f, 0.0f, 1};
    Aircraft aircraft;
    int over = 0;
    int second;

    cruise(&aircraft, 1.0f);
    fly(&aircraft, &burner, 5.0f);
    check(aircraft.speed > 320.0f && aircraft.speed < 360.0f, "the afterburner pushes past full throttle at once");
    fly(&aircraft, &burner, 85.0f);
    check_close(aircraft.speed, 400.0f, 5.0f, "and settles at the top speed");
    check(aircraft.afterburner, "and the jet knows it is lit");

    aim(&aircraft, -60.0f);
    for (second = 0; second < 30; second++) {
        fly(&aircraft, &burner, 1.0f);
        over |= aircraft.speed > 400.0f;
    }
    check(!over, "a dive on the afterburner never breaks the top speed");

    fly(&aircraft, &hands_off, 1.0f);
    check(!aircraft.afterburner, "releasing the key puts the burner out");
}

static void test_dive_and_climb(void)
{
    Aircraft aircraft;

    cruise(&aircraft, 0.7f);
    aim(&aircraft, -30.0f);
    fly(&aircraft, &hands_off, 10.0f);
    check(aircraft.speed > CRUISE_SPEED + 20.0f, "a dive picks up speed");
    check(aircraft.position.y < CRUISE_ALTITUDE, "and loses height");

    cruise(&aircraft, 0.7f);
    aim(&aircraft, 30.0f);
    fly(&aircraft, &hands_off, 10.0f);
    check(aircraft.speed < CRUISE_SPEED - 20.0f, "a climb costs speed");
    check(aircraft.position.y > CRUISE_ALTITUDE, "and buys height");
}

static void test_stall(void)
{
    const AircraftControls pull_up = {1.0f, 0.0f, 0.0f, 0};
    Aircraft aircraft;

    aircraft_place(&aircraft, v3(0.0f, CRUISE_ALTITUDE, 0.0f), 0.0f, 60.0f, 0.0f);
    fly(&aircraft, &pull_up, 2.0f);
    check(climb_angle(&aircraft) < -10.0f, "below the stall speed the nose falls however hard the stick is pulled");

    cruise(&aircraft, 0.7f);
    fly(&aircraft, &pull_up, 1.0f);
    check(climb_angle(&aircraft) > 20.0f, "at cruise the same pull climbs at the full rate");
}

static void test_ceiling(void)
{
    Aircraft aircraft;

    aircraft_place(&aircraft, v3(0.0f, 4400.0f, 0.0f), 0.0f, CRUISE_SPEED, 1.0f);
    aim(&aircraft, 20.0f);
    fly(&aircraft, &hands_off, 60.0f);
    check(aircraft.position.y > 4400.0f, "the jet still climbs below the ceiling");
    check(aircraft.position.y <= 5000.0f, "and stops at five kilometres");

    aircraft.position.y = 5000.0f;
    fly(&aircraft, &hands_off, 5.0f);
    check_close(aircraft.position.y, 5000.0f, 0.001f, "a nose up above the ceiling buys no height at all");
}

static void test_turn(void)
{
    const AircraftControls bank_right = {0.0f, 1.0f, 0.0f, 0};
    Aircraft aircraft;

    cruise(&aircraft, 0.7f);
    fly(&aircraft, &bank_right, 3.0f);
    check_close(aircraft_bank(&aircraft) / VEC_DEGREES, 70.0f, 1.0f, "the bank stops at the limit");
    check(aircraft_heading(&aircraft) > 2.0f * VEC_DEGREES, "and the banked jet turns to the right");

    fly(&aircraft, &hands_off, 3.0f);
    check_close(aircraft_bank(&aircraft) / VEC_DEGREES, 0.0f, 1.0f, "releasing the key levels the wings");
}

static void test_stick_directions(void)
{
    const AircraftControls push_down = {-1.0f, 0.0f, 0.0f, 0};
    const AircraftControls pull_up = {1.0f, 0.0f, 0.0f, 0};
    const AircraftControls rudder_right = {0.0f, 0.0f, 1.0f, 0};
    Aircraft aircraft;

    cruise(&aircraft, 0.7f);
    fly(&aircraft, &pull_up, 1.0f);
    check(climb_angle(&aircraft) > 0.0f, "a stick pulled back raises the nose");

    cruise(&aircraft, 0.7f);
    fly(&aircraft, &push_down, 1.0f);
    check(climb_angle(&aircraft) < 0.0f, "and pushed forward drops it, which is what inverting the keys gives");

    cruise(&aircraft, 0.7f);
    fly(&aircraft, &rudder_right, 1.0f);
    check(aircraft_heading(&aircraft) > 0.0f, "the rudder pushes the nose to the right");
    check(fabsf(aircraft_bank(&aircraft)) < 1.0f * VEC_DEGREES, "without banking the jet");
}

static void test_heading(void)
{
    Aircraft aircraft;

    aircraft_place(&aircraft, v3(0.0f, CRUISE_ALTITUDE, 0.0f), 0.0f, CRUISE_SPEED, 0.7f);
    check_close(aircraft_heading(&aircraft), 0.0f, 1e-5f, "heading zero is north");
    fly(&aircraft, &hands_off, 1.0f);
    check(aircraft.position.z > 200.0f, "and flies toward +z");

    aircraft_place(&aircraft, v3(0.0f, CRUISE_ALTITUDE, 0.0f), 90.0f * VEC_DEGREES, CRUISE_SPEED, 0.7f);
    check_close(aircraft_heading(&aircraft) / VEC_DEGREES, 90.0f, 1e-3f, "ninety degrees is east");
    fly(&aircraft, &hands_off, 1.0f);
    check(aircraft.position.x < 0.0f, "which is -x in this world");
}

static void test_throttle(void)
{
    Aircraft aircraft;

    cruise(&aircraft, 0.7f);
    aircraft_throttle(&aircraft, 1);
    check_close(aircraft.throttle, 0.8f, 1e-5f, "one notch moves the throttle by a tenth");
    aircraft_throttle(&aircraft, -3);
    check_close(aircraft.throttle, 0.5f, 1e-5f, "and three notches back by three tenths");

    aircraft_throttle(&aircraft, 20);
    check_close(aircraft.throttle, 1.0f, 1e-5f, "the throttle stops at full");
    aircraft_throttle(&aircraft, -30);
    check_close(aircraft.throttle, 0.0f, 1e-5f, "and at idle");
}

void test_aircraft_main(void)
{
    test_trim_speeds();
    test_afterburner();
    test_dive_and_climb();
    test_stall();
    test_ceiling();
    test_turn();
    test_stick_directions();
    test_heading();
    test_throttle();
}
