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
    // NULL when the game has nothing to say
    const char *message;
    const char *hint;
} HudState;

void hud_draw(Text *text, const HudState *state, int width, int height);

#endif
