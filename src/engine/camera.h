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

// a fly camera for development: yaw 0 looks toward +z and grows toward +x, pitch raises the view
typedef struct {
    Vec3 position;
    float yaw;
    float pitch;
} FlyCamera;

// the chase camera holds its pose relative to the aircraft, so a straight run has no lag and only a maneuver
// moves the jet in the frame
typedef struct {
    Vec3 eye_offset;
    Vec3 target_offset;
    Vec3 up;
} ChaseCamera;

Mat4 camera_view(const Camera *camera);
Mat4 camera_projection(const Camera *camera, int width, int height);
// eye on a sphere around target: yaw 0 looks from +z, yaw grows toward +x, pitch raises the eye
void camera_orbit(Camera *camera, Vec3 target, float yaw, float pitch, float distance);
// move is in the camera's own frame: x to the right, y up, z along the view, in metres
void camera_fly_step(FlyCamera *fly, Vec3 move, float yaw_delta, float pitch_delta);
void camera_fly(Camera *camera, const FlyCamera *fly);
// skips the easing, for the frame a flight opens on
void camera_chase_settle(ChaseCamera *chase, Quat orientation);
void camera_chase_step(ChaseCamera *chase, Quat orientation, float dt);
void camera_chase(Camera *camera, const ChaseCamera *chase, Vec3 position);
// the eye rides a body at an offset given in the body's own frame and looks along its nose
void camera_attached(Camera *camera, Vec3 position, Quat orientation, Vec3 offset);

#endif
