#include "check.h"
#include "engine/camera.h"
#include "engine/vecmath.h"

void test_camera_main(void)
{
    Camera camera;
    Mat4 view;
    Mat4 projection;
    Vec3 seen;

    camera.eye = v3(0.0f, 0.0f, 4.0f);
    camera.target = v3(0.0f, 0.0f, 0.0f);
    camera.up = v3(0.0f, 1.0f, 0.0f);
    camera.fov_y_radians = VEC_PI * 0.5f;
    camera.near_plane = 1.0f;
    camera.far_plane = 10.0f;

    view = camera_view(&camera);
    seen = m4_transform_point(view, v3(1.0f, 0.0f, 0.0f));
    check_close(seen.x, 1.0f, 1e-6f, "view keeps x");
    check_close(seen.z, -4.0f, 1e-6f, "view moves the origin to -4 on z");

    projection = camera_projection(&camera, 1600, 800);
    check_close(projection.m[0], 0.5f, 1e-6f, "projection uses the window aspect");
    check_close(camera_projection(&camera, 800, 0).m[0], 1.0f, 1e-6f, "zero height falls back to a square aspect");
}
