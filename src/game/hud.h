#ifndef HUD_H
#define HUD_H

#include "engine/text.h"

typedef struct {
    // the flight instruments only mean anything once the jet has left its parking spot
    int in_flight;
    // metres per second and metres, turned into the units on screen by the HUD itself
    float speed;
    float altitude;
    // degrees clockwise from north
    float heading;
    float throttle;
    int afterburner;
    // missiles hanging on the pylons, and how many of them there are
    int missiles;
    int pylons;
    int targets_destroyed;
    int targets_total;
    // the designated target's bearing off the boresight (radians, 0 up, growing clockwise) and range in
    // metres; both meaningless while has_target is false
    int has_target;
    float target_bearing;
    float target_range;
    // the same target's box projected to the screen, drawn only while locked is true
    int locked;
    float lock_x0;
    float lock_y0;
    float lock_x1;
    float lock_y1;
    // the terrain ahead is closing in on the flight path
    int pull_up;
    // NULL when the game has nothing to say
    const char *message;
    const char *hint;
} HudState;

void hud_draw(Text *text, const HudState *state, int width, int height);

#endif
