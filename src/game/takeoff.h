#ifndef TAKEOFF_H
#define TAKEOFF_H

#include "engine/vecmath.h"
#include "game/aircraft.h"

#define TAKEOFF_LEGS 2

typedef enum {
    TAKEOFF_TAXI,
    TAKEOFF_ROLL,
    TAKEOFF_CLIMB,
    TAKEOFF_DONE
} TakeoffPhase;

typedef struct {
    TakeoffPhase phase;
    float time;
    // the two taxiway corners and the one being steered for; past them the jet aims up the centerline itself
    Vec3 legs[TAKEOFF_LEGS];
    int leg;
    float heading;
    float pitch;
    // 0 leaves the canopy wide open, 1 shuts it
    float canopy;
    int gear_up;
} Takeoff;

// the script flies the jet itself from wherever it stands, and the flight model takes over once it is done
void takeoff_begin(Takeoff *takeoff, Aircraft *aircraft);
void takeoff_step(Takeoff *takeoff, Aircraft *aircraft, float dt);

#endif
