#include "engine/camera.h"

#include <math.h>

// straight up would leave the view without a horizon and the yaw without meaning
#define FLY_MAX_PITCH (85.0f * VEC_DEGREES)

// behind and above the tail, aiming a little ahead of the nose so the jet sits below the middle of the frame
#define CHASE_DISTANCE 55.0f
#define CHASE_HEIGHT 13.0f
#define CHASE_AIM_AHEAD 25.0f
#define CHASE_BANK_SHARE 0.35f
// how long the camera takes to catch up with a maneuver, in seconds
#define CHASE_MOVE_TIME 0.25f
#define CHASE_AIM_TIME 0.15f
#define CHASE_BANK_TIME 0.35f

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
    fly->pitch = clamped(pitch, -FLY_MAX_PITCH, FLY_MAX_PITCH);
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

static void chase_pose(Quat orientation, Vec3 *eye_offset, Vec3 *target_offset, Vec3 *up)
{
    const Vec3 forward = quat_rotate(orientation, v3(0.0f, 0.0f, 1.0f));
    const Vec3 body_up = quat_rotate(orientation, v3(0.0f, 1.0f, 0.0f));

    *eye_offset = v3_add(v3_scale(forward, -CHASE_DISTANCE), v3(0.0f, CHASE_HEIGHT, 0.0f));
    *target_offset = v3_scale(forward, CHASE_AIM_AHEAD);
    // the share of the bank the camera copies also keeps the up vector off the view direction in a vertical climb
    *up = v3_normalize(v3_lerp(v3(0.0f, 1.0f, 0.0f), body_up, CHASE_BANK_SHARE));
}

void camera_chase_settle(ChaseCamera *chase, Quat orientation)
{
    chase_pose(orientation, &chase->eye_offset, &chase->target_offset, &chase->up);
}

void camera_chase_step(ChaseCamera *chase, Quat orientation, float dt)
{
    Vec3 eye_offset;
    Vec3 target_offset;
    Vec3 up;

    chase_pose(orientation, &eye_offset, &target_offset, &up);
    chase->eye_offset = v3_lerp(chase->eye_offset, eye_offset, 1.0f - expf(-dt / CHASE_MOVE_TIME));
    chase->target_offset = v3_lerp(chase->target_offset, target_offset, 1.0f - expf(-dt / CHASE_AIM_TIME));
    chase->up = v3_normalize(v3_lerp(chase->up, up, 1.0f - expf(-dt / CHASE_BANK_TIME)));
}

void camera_chase(Camera *camera, const ChaseCamera *chase, Vec3 position)
{
    camera->eye = v3_add(position, chase->eye_offset);
    camera->target = v3_add(position, chase->target_offset);
    camera->up = chase->up;
}

void camera_attached(Camera *camera, Vec3 position, Quat orientation, Vec3 offset)
{
    camera->eye = v3_add(position, quat_rotate(orientation, offset));
    camera->target = v3_add(camera->eye, quat_rotate(orientation, v3(0.0f, 0.0f, 1.0f)));
    camera->up = quat_rotate(orientation, v3(0.0f, 1.0f, 0.0f));
}
