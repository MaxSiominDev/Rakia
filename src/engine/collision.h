#ifndef COLLISION_H
#define COLLISION_H

#include "engine/picking.h"
#include "engine/vecmath.h"

// what an object standing on the apron covers: a box around its center, turned about +y
typedef struct {
    Vec3 center;
    float half_x;
    float half_z;
    float yaw;
} Footprint;

// the whole box a model fills: its own bounds in metres, standing at position and turned about +y by yaw
typedef struct {
    Vec3 position;
    float yaw;
    Vec3 min;
    Vec3 max;
} Hitbox;

// the ground box of a model whose own bounds are min to max, placed at position and turned by yaw
Footprint collision_footprint(Vec3 position, float yaw, Vec3 min, Vec3 max);
// the shortest move that takes a clear of b, in the ground plane; 0 and *push untouched when they are apart
int collision_push(const Footprint *a, const Footprint *b, Vec3 *push);
// 1 when the segment from one point to the other, swept by a ball of that radius, touches the box
int collision_segment(const Hitbox *box, Vec3 from, Vec3 to, float radius);

#endif
