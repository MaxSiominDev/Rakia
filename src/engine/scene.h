#ifndef SCENE_H
#define SCENE_H

#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/vecmath.h"

#define SCENE_MAX_ENTITIES 64
#define SCENE_MAX_GROUPS 128

enum {
    GROUP_HIDDEN = 1,
    GROUP_BLENDED = 2,
    GROUP_DOUBLE_SIDED = 4,
    // the group rides Entity.hinge instead of sitting still in the model
    GROUP_HINGED = 8
};

typedef struct {
    Vec3 position;
    Quat orientation;
    float scale;
    const Mesh *mesh;
    // indexed by the mesh groups' material numbers
    const Material *materials;
    unsigned char group_flags[SCENE_MAX_GROUPS];
    // where the hinged groups go in the model's own frame: the canopy swinging open
    Mat4 hinge;
    // a hidden entity is still a shadow caster, which is what the view from inside the cockpit needs
    int hidden;
    int casts_shadow;
    // the tint the hangar puts on picked objects, mixed over the diffuse color by highlight (0 to 1)
    Vec3 highlight_color;
    float highlight;
} Entity;

typedef struct {
    Entity entities[SCENE_MAX_ENTITIES];
    int entity_count;
} Scene;

// at the origin, unturned, scale 1, every group visible and opaque, casting a shadow; NULL when the scene is full
// or the mesh has more groups than an entity can flag
Entity *scene_add(Scene *scene, const Mesh *mesh, const Material *materials);
Mat4 entity_matrix(const Entity *entity);

#endif
