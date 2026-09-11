#include "engine/camera.h"

Mat4 camera_view(const Camera *camera)
{
    return m4_look_at(camera->eye, camera->target, camera->up);
}

Mat4 camera_projection(const Camera *camera, int width, int height)
{
    const float aspect = width > 0 && height > 0 ? (float)width / (float)height : 1.0f;

    return m4_perspective(camera->fov_y_radians, aspect, camera->near_plane, camera->far_plane);
}
