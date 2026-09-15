#include "game/takeoff.h"

#include "game/world.h"

#include <math.h>

#define TAXI_SPEED 24.0f
#define TAXI_TURN (40.0f * VEC_DEGREES)
// how close the jet has to come to a waypoint before it steers for the next one; a taxi turn is wider than
// this, so a waypoint that ends up behind the nose counts as reached too, or the jet would circle it forever
#define TAXI_REACH 15.0f
// how far up the centerline the jet aims once it is off the taxiway, which sets how hard it closes on the line
#define LINEUP_AHEAD 120.0f
// lined up once the centerline is this close and the nose this near the runway heading
#define LINEUP_OFFSET 6.0f
#define LINEUP_HEADING (4.0f * VEC_DEGREES)
#define ROLL_ACCEL 12.0f
#define ROTATE_SPEED 80.0f
#define ROTATE_RATE (7.0f * VEC_DEGREES)
#define CLIMB_PITCH (13.0f * VEC_DEGREES)
#define CLIMB_SPEED 165.0f
#define GEAR_UP_HEIGHT 40.0f
#define HANDOVER_HEIGHT 150.0f
// the setting the flight model inherits, which trims out well above the speed it is handed
#define HANDOVER_THROTTLE 0.85f
#define CANOPY_SECONDS 2.5f

// the compass heading of a step across the ground, counted the way the aircraft counts it
static float compass(float dx, float dz)
{
    return atan2f(-dx, dz);
}

// the signed turn from one heading to another, the short way round
static float heading_delta(float from, float to)
{
    float difference = to - from;

    while (difference > VEC_PI) {
        difference -= 2.0f * VEC_PI;
    }
    while (difference < -VEC_PI) {
        difference += 2.0f * VEC_PI;
    }

    return difference;
}

static float turn_toward(float heading, float wanted, float most)
{
    return heading + clamped(heading_delta(heading, wanted), -most, most);
}

static Quat pose(const Takeoff *takeoff)
{
    const Quat turn = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), -takeoff->heading);
    const Quat nose = quat_from_axis_angle(v3(1.0f, 0.0f, 0.0f), -takeoff->pitch);

    return quat_multiply(turn, nose);
}

// the heading that closes on the centerline from wherever the jet is, aiming a fixed way up the runway
static float centerline(const Aircraft *aircraft)
{
    return compass(WORLD_RUNWAY_X - aircraft->position.x, LINEUP_AHEAD);
}

static int lined_up(const Takeoff *takeoff, const Aircraft *aircraft)
{
    return takeoff->leg == TAKEOFF_LEGS && fabsf(aircraft->position.x - WORLD_RUNWAY_X) < LINEUP_OFFSET &&
           fabsf(heading_delta(takeoff->heading, WORLD_RUNWAY_HEADING)) < LINEUP_HEADING;
}

static void taxi(Takeoff *takeoff, Aircraft *aircraft, float dt)
{
    const int on_taxiway = takeoff->leg < TAKEOFF_LEGS;
    const float dx = on_taxiway ? takeoff->legs[takeoff->leg].x - aircraft->position.x : 0.0f;
    const float dz = on_taxiway ? takeoff->legs[takeoff->leg].z - aircraft->position.z : 0.0f;
    const float ahead = dz * cosf(takeoff->heading) - dx * sinf(takeoff->heading);

    aircraft->speed = TAXI_SPEED;
    takeoff->heading = turn_toward(takeoff->heading, on_taxiway ? compass(dx, dz) : centerline(aircraft),
                                   TAXI_TURN * dt);
    if (on_taxiway && (dx * dx + dz * dz < TAXI_REACH * TAXI_REACH || ahead <= 0.0f)) {
        takeoff->leg++;
    }
    if (lined_up(takeoff, aircraft)) {
        takeoff->phase = TAKEOFF_ROLL;
    }
}

static void roll(Takeoff *takeoff, Aircraft *aircraft, float dt)
{
    aircraft->speed += ROLL_ACCEL * dt;
    takeoff->heading = turn_toward(takeoff->heading, centerline(aircraft), TAXI_TURN * dt);
    if (aircraft->speed >= ROTATE_SPEED) {
        takeoff->phase = TAKEOFF_CLIMB;
    }
}

static void climb(Takeoff *takeoff, Aircraft *aircraft, float dt)
{
    const float height = aircraft->position.y - WORLD_STAND;

    aircraft->speed = fminf(aircraft->speed + ROLL_ACCEL * dt, CLIMB_SPEED);
    takeoff->pitch = fminf(takeoff->pitch + ROTATE_RATE * dt, CLIMB_PITCH);
    takeoff->gear_up = height > GEAR_UP_HEIGHT;
    if (height >= HANDOVER_HEIGHT) {
        takeoff->phase = TAKEOFF_DONE;
    }
}

void takeoff_begin(Takeoff *takeoff, Aircraft *aircraft)
{
    takeoff->phase = TAKEOFF_TAXI;
    takeoff->time = 0.0f;
    takeoff->legs[0] = v3(aircraft->position.x, 0.0f, WORLD_TAXI_Z);
    takeoff->legs[1] = v3(WORLD_TAXI_CORNER_X, 0.0f, WORLD_TAXI_Z);
    takeoff->leg = 0;
    takeoff->heading = aircraft_heading(aircraft);
    takeoff->pitch = 0.0f;
    takeoff->canopy = 0.0f;
    takeoff->gear_up = 0;
    aircraft->speed = TAXI_SPEED;
    aircraft->throttle = HANDOVER_THROTTLE;
    aircraft->afterburner = 0;
}

void takeoff_step(Takeoff *takeoff, Aircraft *aircraft, float dt)
{
    takeoff->time += dt;
    takeoff->canopy = clamped(takeoff->time / CANOPY_SECONDS, 0.0f, 1.0f);

    switch (takeoff->phase) {
    case TAKEOFF_TAXI:
        taxi(takeoff, aircraft, dt);
        break;
    case TAKEOFF_ROLL:
        roll(takeoff, aircraft, dt);
        break;
    case TAKEOFF_CLIMB:
        climb(takeoff, aircraft, dt);
        break;
    case TAKEOFF_DONE:
        return;
    }

    aircraft->orientation = pose(takeoff);
    aircraft->position = v3_add(aircraft->position,
                                v3_scale(quat_rotate(aircraft->orientation, v3(0.0f, 0.0f, 1.0f)),
                                         aircraft->speed * dt));
}
