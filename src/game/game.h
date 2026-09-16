#ifndef GAME_H
#define GAME_H

#include "engine/camera.h"
#include "engine/input.h"
#include "engine/light.h"
#include "engine/particles.h"
#include "engine/renderer.h"
#include "engine/scene.h"
#include "engine/text.h"
#include "game/aircraft.h"
#include "game/effects.h"
#include "game/hangar.h"
#include "game/takeoff.h"
#include "game/targets.h"
#include "game/weapons.h"
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
    Particles particles;
    Effects effects;
    Targets targets;
    Weapons weapons;
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
    // the target Tab has picked, -1 for the automatic nearest-live one
    int designated;
    // counts up on every hangar entry, which is what changes the target layout's seed
    unsigned int mission;
    // world clock reading when the flight took over from the takeoff script, for the mission timer
    float mission_start;
    float mission_time;
} Game;

// the platform hands the context back as a void pointer, hence the signatures
int game_init(void *context);
void game_step(void *context, const Input *input, float dt);
void game_render(void *context, int width, int height);

typedef struct {
    // 1 when the projection lands inside the inset frame
    int on_screen;
    float x;
    float y;
    // the outward direction for an edge marker: radians, 0 up, growing clockwise; meaningless when on_screen
    float angle;
} TargetMark;

TargetMark locate_target_mark(const Camera *camera, Vec3 point, int width, int height, float margin_x, float margin_y);

#endif
