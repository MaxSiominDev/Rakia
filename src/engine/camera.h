#ifndef CAMERA_H
#define CAMERA_H

#include "engine/vecmath.h"

typedef struct {
    Vec3 eye;
    Vec3 target;
    Vec3 up;
    float fov_y_radians;
    float near_plane;
    float far_plane;
} Camera;

Mat4 camera_view(const Camera *camera);
Mat4 camera_projection(const Camera *camera, int width, int height);
// eye on a sphere around target: yaw 0 looks from +z, yaw grows toward +x, pitch raises the eye
void camera_orbit(Camera *camera, Vec3 target, float yaw, float pitch, float distance);

#endif
