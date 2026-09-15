#ifndef HUD_H
#define HUD_H

#include "engine/text.h"

typedef struct {
    // metres per second and metres, turned into the units on screen by the HUD itself
    float speed;
    float altitude;
    // degrees clockwise from north
    float heading;
    float throttle;
    int afterburner;
    // NULL when the game has nothing to say
    const char *message;
    const char *hint;
} HudState;

void hud_draw(Text *text, const HudState *state, int width, int height);

#endif
