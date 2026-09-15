#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "engine/vecmath.h"

typedef struct {
    Vec3 position;
    Quat orientation;
    // along the nose, in metres per second
    float speed;
    // 0 to 1
    float throttle;
    int afterburner;
} Aircraft;

// each from -1 to 1: the stick pulled back, the bank asked for to the right, the rudder to the right
typedef struct {
    float pitch;
    float bank;
    float rudder;
    int afterburner;
} AircraftControls;

// heading in radians clockwise from north, the way the compass and the HUD count it
void aircraft_place(Aircraft *aircraft, Vec3 position, float heading, float speed, float throttle);
// one fixed simulation step; the model is not written for a variable dt
void aircraft_step(Aircraft *aircraft, const AircraftControls *controls, float dt);
void aircraft_throttle(Aircraft *aircraft, int steps);
float aircraft_heading(const Aircraft *aircraft);
// the wings' angle from level, positive with the right wing down
float aircraft_bank(const Aircraft *aircraft);

#endif
