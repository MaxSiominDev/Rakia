#ifndef GAME_H
#define GAME_H

#include "engine/camera.h"
#include "engine/input.h"
#include "engine/light.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/renderer.h"
#include "engine/scene.h"
#include "engine/text.h"
#include "game/aircraft.h"

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
    Aircraft aircraft;
    Renderer renderer;
    Scene scene;
    Camera camera;
    Light light;
    Text hud;
    ChaseCamera chase;
    FlyCamera fly;
    Entity *jet;
    Mesh jet_mesh;
    Material *jet_materials;
    Mesh apron_mesh;
    Material apron_material;
    // in the jet's own frame, scaled to metres: the box the ground contact walks, the point the paused camera
    // orbits and the pilot's eye
    Vec3 jet_min;
    Vec3 jet_max;
    Vec3 jet_center;
    Vec3 cockpit_eye;
    int cockpit_view;
    int inverted_pitch;
    // seconds the note about the pitch keys stays on the HUD
    float invert_note;
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
