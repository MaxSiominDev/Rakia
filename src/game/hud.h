#ifndef HUD_H
#define HUD_H

#include "engine/text.h"

// HUD sizes are given in points on a screen this many lines tall
#define HUD_REFERENCE_HEIGHT 720.0f

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
    // meaningless while has_target is false
    int has_target;
    // 1 for a diamond at target_x, target_y; 0 for an edge arrow there along target_angle, radians clockwise from up
    int target_on_screen;
    float target_x;
    float target_y;
    float target_angle;
    // metres
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
