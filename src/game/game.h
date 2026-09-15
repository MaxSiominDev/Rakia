#ifndef GAME_H
#define GAME_H

#include "engine/camera.h"
#include "engine/input.h"
#include "engine/light.h"
#include "engine/renderer.h"
#include "engine/scene.h"
#include "engine/text.h"
#include "game/aircraft.h"
#include "game/hangar.h"
#include "game/takeoff.h"
#include "game/world.h"

typedef enum {
    GAME_HANGAR,
    GAME_TAKEOFF,
    GAME_FLIGHT,
    GAME_PAUSE,
    GAME_CRASH,
    GAME_COMPLETE
} GameState;

typedef struct {
    GameState state;
    // what the pause opened from, which is also what the world still looks like while it is up
    GameState resume;
    Aircraft aircraft;
    Renderer renderer;
    Scene scene;
    Camera camera;
    Light light;
    Text hud;
    ChaseCamera chase;
    FlyCamera fly;
    World world;
    Hangar hangar;
    Takeoff takeoff;
    int cockpit_view;
    int inverted_pitch;
    // seconds left on the note about the pitch keys, and on the wreck before the hangar opens again
    float invert_note;
    float crash_wait;
    // the development camera the pause offers instead of the orbit
    int free_camera;
    float view_yaw;
    float view_pitch;
    float view_distance;
    // the keys held one step ago, so a press can be told from a hold
    Input previous;
    float time;
} Game;

// the platform hands the context back as a void pointer, hence the signatures
int game_init(void *context);
void game_step(void *context, const Input *input, float dt);
void game_render(void *context, int width, int height);

#endif
