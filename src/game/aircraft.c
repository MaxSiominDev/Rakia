#include "game/aircraft.h"

#include <math.h>

#define GRAVITY 9.81f
// level flight settles where thrust cancels drag, so a throttle setting is written as the speed it holds
#define IDLE_SPEED 90.0f
#define FULL_SPEED 320.0f
#define AFTERBURNER_SPEED 400.0f
// a dive would otherwise carry the jet past every trim speed
#define MAX_SPEED 400.0f
// a stalled jet still falls forward, and the turn rate divides by the speed
#define MIN_SPEED 20.0f
// drag grows with the square of the speed; this factor also sets how long the jet takes to answer the throttle
#define DRAG 2.0e-4f
#define STALL_SPEED 80.0f
// the wings have their full bite from here up, and the stick fades with the square of the speed below it
#define CONTROL_SPEED 140.0f
#define STALL_NOSE_DROP (60.0f * VEC_DEGREES)
#define PITCH_RATE (30.0f * VEC_DEGREES)
#define ROLL_RATE (90.0f * VEC_DEGREES)
// the wings close the angle between the bank they hold and the one the stick asks for at this rate, so the
// last few degrees ease in instead of stopping dead
#define ROLL_GAIN 3.0f
#define BANK_LIMIT (70.0f * VEC_DEGREES)
#define RUDDER_RATE (6.0f * VEC_DEGREES)
#define CEILING 5000.0f
#define CEILING_FADE 500.0f
#define THROTTLE_STEP 0.1f

static Vec3 aircraft_forward(const Aircraft *aircraft)
{
    return quat_rotate(aircraft->orientation, v3(0.0f, 0.0f, 1.0f));
}

// the air load on a control surface grows with the square of the speed
static float stick_bite(float speed)
{
    const float share = speed / CONTROL_SPEED;

    return clamped(share * share, 0.0f, 1.0f);
}

void aircraft_place(Aircraft *aircraft, Vec3 position, float heading, float speed, float throttle)
{
    aircraft->position = position;
    // the compass runs clockwise while a turn about +y takes the nose from north toward the west
    aircraft->orientation = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), -heading);
    aircraft->speed = clamped(speed, MIN_SPEED, MAX_SPEED);
    aircraft->throttle = clamped(throttle, 0.0f, 1.0f);
    aircraft->afterburner = 0;
}

void aircraft_step(Aircraft *aircraft, const AircraftControls *controls, float dt)
{
    const Vec3 forward = aircraft_forward(aircraft);
    const float bank = aircraft_bank(aircraft);
    const float bite = stick_bite(aircraft->speed);
    // a bank angle means nothing with the nose straight up or down, so both the wings' answer to it and the
    // turn it would bend the flight path into fade out as the climb goes vertical
    const float level = v3_length(v3(forward.x, 0.0f, forward.z));
    const float turn_rate = GRAVITY * tanf(clamped(bank, -BANK_LIMIT, BANK_LIMIT)) * level / aircraft->speed;
    const float trim = controls->afterburner ? AFTERBURNER_SPEED
                                             : IDLE_SPEED + (FULL_SPEED - IDLE_SPEED) * aircraft->throttle;
    const float nose_drop = STALL_NOSE_DROP * (1.0f - clamped(aircraft->speed / STALL_SPEED, 0.0f, 1.0f));
    const float pitch_rate = PITCH_RATE * controls->pitch * bite - nose_drop;
    const float roll_rate =
        clamped((BANK_LIMIT * controls->bank - bank) * ROLL_GAIN, -ROLL_RATE, ROLL_RATE) * bite * level;
    const float yaw_rate = RUDDER_RATE * controls->rudder * bite;
    // the jet's own axes are +x along the left wing, +y up and +z along the nose, so raising the nose turns
    // about -x and a rudder kick to the right about -y
    const Vec3 body_rate = v3(-pitch_rate, -yaw_rate, roll_rate);
    const Quat spin = quat_from_axis_angle(body_rate, v3_length(body_rate) * dt);
    const Quat turn = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), -turn_rate * dt);
    Vec3 velocity;

    aircraft->afterburner = controls->afterburner;
    aircraft->speed = clamped(aircraft->speed +
                              (DRAG * (trim * trim - aircraft->speed * aircraft->speed) - GRAVITY * forward.y) * dt,
                              MIN_SPEED, MAX_SPEED);
    aircraft->orientation = quat_normalize(quat_multiply(turn, quat_multiply(aircraft->orientation, spin)));

    velocity = v3_scale(aircraft_forward(aircraft), aircraft->speed);
    if (velocity.y > 0.0f) {
        velocity.y *= clamped((CEILING - aircraft->position.y) / CEILING_FADE, 0.0f, 1.0f);
    }
    aircraft->position = v3_add(aircraft->position, v3_scale(velocity, dt));
}

void aircraft_throttle(Aircraft *aircraft, int steps)
{
    aircraft->throttle = clamped(aircraft->throttle + THROTTLE_STEP * (float)steps, 0.0f, 1.0f);
}

float aircraft_heading(const Aircraft *aircraft)
{
    const Vec3 forward = aircraft_forward(aircraft);
    const float heading = atan2f(-forward.x, forward.z);

    return heading < 0.0f ? heading + 2.0f * VEC_PI : heading;
}

float aircraft_bank(const Aircraft *aircraft)
{
    const Vec3 up = v3(0.0f, 1.0f, 0.0f);
    const Vec3 left_wing = quat_rotate(aircraft->orientation, v3(1.0f, 0.0f, 0.0f));
    // where the left wing points with the wings level, which straight up or down is nothing at all
    const Vec3 level = v3_normalize(v3_cross(up, aircraft_forward(aircraft)));

    return atan2f(v3_dot(left_wing, up), v3_dot(left_wing, level));
}
