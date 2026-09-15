#include "check.h"
#include "engine/camera.h"
#include "engine/vecmath.h"

static void test_view_and_projection(void)
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

static void test_orbit(void)
{
    const Vec3 target = v3(1.0f, 2.0f, 3.0f);
    Camera camera;

    camera_orbit(&camera, target, 0.0f, 0.0f, 5.0f);
    check_close(camera.eye.x, 1.0f, 1e-5f, "yaw 0 keeps x");
    check_close(camera.eye.y, 2.0f, 1e-5f, "pitch 0 keeps y");
    check_close(camera.eye.z, 8.0f, 1e-5f, "yaw 0 puts the eye on the +z side");
    check(camera.target.x == 1.0f && camera.target.y == 2.0f && camera.target.z == 3.0f, "orbit looks at the target");
    check(camera.up.y == 1.0f, "orbit keeps y up");

    camera_orbit(&camera, target, VEC_PI * 0.5f, 0.0f, 5.0f);
    check_close(camera.eye.x, 6.0f, 1e-5f, "yaw 90 degrees puts the eye on the +x side");
    check_close(camera.eye.z, 3.0f, 1e-5f, "yaw 90 degrees leaves z at the target");

    camera_orbit(&camera, target, 0.0f, VEC_PI / 6.0f, 4.0f);
    check_close(camera.eye.y, 4.0f, 1e-5f, "pitch 30 degrees raises the eye by half the distance");
    check_close(v3_length(v3_sub(camera.eye, target)), 4.0f, 1e-5f, "the eye stays at the given distance");
}

static void test_fly(void)
{
    const float quarter = VEC_PI * 0.5f;
    Camera camera;
    FlyCamera fly;

    fly.position = v3(0.0f, 100.0f, 0.0f);
    fly.yaw = 0.0f;
    fly.pitch = 0.0f;

    camera_fly_step(&fly, v3(0.0f, 0.0f, 10.0f), 0.0f, 0.0f);
    check_v3(fly.position, 0.0f, 100.0f, 10.0f, "yaw 0 flies north");
    camera_fly_step(&fly, v3(4.0f, 0.0f, 0.0f), 0.0f, 0.0f);
    check_v3(fly.position, -4.0f, 100.0f, 10.0f, "facing north, moving right goes east");
    camera_fly_step(&fly, v3(0.0f, -6.0f, 0.0f), 0.0f, 0.0f);
    check_v3(fly.position, -4.0f, 94.0f, 10.0f, "up and down follow the world, not the view");

    camera_fly_step(&fly, v3(0.0f, 0.0f, 5.0f), quarter, 0.0f);
    check_v3(fly.position, 1.0f, 94.0f, 10.0f, "a quarter turn to the left points the view west");

    fly.yaw = 0.0f;
    camera_fly_step(&fly, v3(0.0f, 0.0f, 0.0f), 0.0f, 10.0f * quarter);
    check_close(fly.pitch, 85.0f * VEC_DEGREES, 1e-5f, "the view stops short of straight up");
    camera_fly_step(&fly, v3(0.0f, 0.0f, 0.0f), 0.0f, -20.0f * quarter);
    check_close(fly.pitch, -85.0f * VEC_DEGREES, 1e-5f, "and short of straight down");

    fly.position = v3(1.0f, 2.0f, 3.0f);
    fly.yaw = 0.0f;
    fly.pitch = 0.0f;
    camera_fly(&camera, &fly);
    check_v3(camera.eye, 1.0f, 2.0f, 3.0f, "the camera sits where the fly camera is");
    check_v3(v3_sub(camera.target, camera.eye), 0.0f, 0.0f, 1.0f, "and looks along its yaw");
    check(camera.up.y == 1.0f, "with the world up");
}

void test_camera_main(void)
{
    test_view_and_projection();
    test_orbit();
    test_fly();
}
