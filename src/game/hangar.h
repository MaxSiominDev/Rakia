#ifndef HANGAR_H
#define HANGAR_H

#include "engine/picking.h"
#include "game/world.h"

typedef struct {
    // the prop under the cursor and the one the button is holding, -1 for neither
    int hovered;
    int dragged;
    // the height the drag runs at and the step from the point under the cursor to the prop's own origin
    float plane;
    Vec3 grab;
    // the button one step ago, so a press can be told from a hold
    int held;
    // the missile hanging on each pylon, -1 while the pylon is free
    int mounted[WORLD_PYLONS];
} Hangar;

void hangar_init(Hangar *hangar);
// the cursor lets go of everything and every tint goes out, which is what leaving the hangar does
void hangar_release(Hangar *hangar, World *world);
// one step of the cursor: the ray under it and whether the left button is held
void hangar_step(Hangar *hangar, World *world, Ray ray, int button);
// the mounted missiles ride their pylons wherever the jet goes
void hangar_carry(const Hangar *hangar, World *world);
int hangar_missiles(const Hangar *hangar);

#endif
