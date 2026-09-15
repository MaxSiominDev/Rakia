#include "engine/camera.h"

#include <math.h>

// straight up would leave the view without a horizon and the yaw without meaning
#define FLY_MAX_PITCH (85.0f * VEC_DEGREES)

Mat4 camera_view(const Camera *camera)
{
    return m4_look_at(camera->eye, camera->target, camera->up);
}

Mat4 camera_projection(const Camera *camera, int width, int height)
{
    const float aspect = width > 0 && height > 0 ? (float)width / (float)height : 1.0f;

    return m4_perspective(camera->fov_y_radians, aspect, camera->near_plane, camera->far_plane);
}

void camera_orbit(Camera *camera, Vec3 target, float yaw, float pitch, float distance)
{
    const float flat = distance * cosf(pitch);

    camera->eye = v3_add(target, v3(flat * sinf(yaw), distance * sinf(pitch), flat * cosf(yaw)));
    camera->target = target;
    camera->up = v3(0.0f, 1.0f, 0.0f);
}

static Vec3 fly_forward(const FlyCamera *fly)
{
    const float flat = cosf(fly->pitch);

    return v3(flat * sinf(fly->yaw), sinf(fly->pitch), flat * cosf(fly->yaw));
}

void camera_fly_step(FlyCamera *fly, Vec3 move, float yaw_delta, float pitch_delta)
{
    const float pitch = fly->pitch + pitch_delta;
    Vec3 forward;
    Vec3 right;
    Vec3 step;

    fly->yaw += yaw_delta;
    fly->pitch = pitch < -FLY_MAX_PITCH ? -FLY_MAX_PITCH : pitch > FLY_MAX_PITCH ? FLY_MAX_PITCH : pitch;
    forward = fly_forward(fly);
    right = v3_normalize(v3_cross(forward, v3(0.0f, 1.0f, 0.0f)));
    step = v3_add(v3_scale(forward, move.z), v3_add(v3_scale(right, move.x), v3(0.0f, move.y, 0.0f)));
    fly->position = v3_add(fly->position, step);
}

void camera_fly(Camera *camera, const FlyCamera *fly)
{
    camera->eye = fly->position;
    camera->target = v3_add(fly->position, fly_forward(fly));
    camera->up = v3(0.0f, 1.0f, 0.0f);
}
