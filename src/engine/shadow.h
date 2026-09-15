#ifndef SHADOW_H
#define SHADOW_H

#include "engine/gl_compat.h"
#include "engine/vecmath.h"

#define SHADOW_MAP_SIZE 4096
// distance between the taps of the 3x3 filter, in shadow map texels
#define SHADOW_TAP_SPREAD 2.0f

typedef struct {
    GLuint texture;
    GLuint framebuffer;
    // world to light clip space for the depth pass, and world to map texture space for the lookup
    Mat4 view_projection;
    Mat4 texture_matrix;
} Shadow;

int shadow_init(Shadow *shadow);
// orthographic box around a sphere; a receiver beyond its far plane is still shadowed correctly, since the lookup
// clamps its depth to the far plane and a directional shadow reaches all the way
void shadow_fit(Shadow *shadow, Vec3 center, float radius, Vec3 sun_direction);
void shadow_begin(const Shadow *shadow);
void shadow_end(int width, int height);

#endif
