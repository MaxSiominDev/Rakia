#include "check.h"
#include "engine/camera.h"
#include "engine/picking.h"
#include "engine/vecmath.h"
#include "game/game.h"

#include <math.h>

#define WIDTH 1280
#define HEIGHT 720
#define MARGIN 70.0f

static Camera looking_north(void)
{
    Camera camera;

    camera.eye = v3(0.0f, 0.0f, 0.0f);
    camera.target = v3(0.0f, 0.0f, 1.0f);
    camera.up = v3(0.0f, 1.0f, 0.0f);
    camera.fov_y_radians = 60.0f * VEC_DEGREES;
    camera.near_plane = 1.0f;
    camera.far_plane = 20000.0f;

    return camera;
}

static void test_ahead_right(void)
{
    const Camera camera = looking_north();
    // east of the nose (east is -x here) and well inside the cone at this range
    const TargetMark mark = locate_target_mark(&camera, v3(-600.0f, 0.0f, 3000.0f), WIDTH, HEIGHT, MARGIN, MARGIN);

    check(mark.on_screen, "a target ahead and to the right lands inside the frame");
    check(mark.x > (float)WIDTH * 0.5f, "on the right half of the screen");
}

static void test_off_right(void)
{
    const Camera camera = looking_north();
    const TargetMark mark = locate_target_mark(&camera, v3(-8000.0f, 0.0f, 3000.0f), WIDTH, HEIGHT, MARGIN, MARGIN);

    check(!mark.on_screen, "a target far to the right falls outside the frame");
    check_close(mark.x, (float)WIDTH - MARGIN, 1e-2f, "so the arrow sits on the right inset edge");
    check_close(mark.angle, VEC_PI * 0.5f, 1e-3f, "pointing right");
}

static void test_off_left(void)
{
    const Camera camera = looking_north();
    const TargetMark mark = locate_target_mark(&camera, v3(8000.0f, 0.0f, 3000.0f), WIDTH, HEIGHT, MARGIN, MARGIN);

    check(!mark.on_screen, "a target far to the left falls outside the frame");
    check_close(mark.x, MARGIN, 1e-2f, "so the arrow sits on the left inset edge");
    check_close(mark.angle, -VEC_PI * 0.5f, 1e-3f, "pointing left");
}

static void test_behind_right(void)
{
    const Camera camera = looking_north();
    // slightly east of straight behind (east is -x)
    const TargetMark mark = locate_target_mark(&camera, v3(-50.0f, 0.0f, -2000.0f), WIDTH, HEIGHT, MARGIN, MARGIN);

    check(!mark.on_screen, "a target behind the camera is never on screen");
    check_close(mark.x, (float)WIDTH - MARGIN, 1e-2f, "behind and slightly right takes the right edge");
    check_close(mark.y, (float)HEIGHT * 0.5f, 1e-2f, "at mid height, never top or bottom");
}

static void test_behind_left(void)
{
    const Camera camera = looking_north();
    const TargetMark mark = locate_target_mark(&camera, v3(50.0f, 0.0f, -2000.0f), WIDTH, HEIGHT, MARGIN, MARGIN);

    check(!mark.on_screen, "a target behind the camera is never on screen");
    check_close(mark.x, MARGIN, 1e-2f, "behind and slightly left takes the left edge");
    check_close(mark.y, (float)HEIGHT * 0.5f, 1e-2f, "at mid height, never top or bottom");
}

static void test_banked(void)
{
    Camera camera = looking_north();
    const Vec3 point = v3(-400.0f, 200.0f, 4000.0f);
    Mat4 view_projection;
    TargetMark mark;
    float expected_x;
    float expected_y;

    // a chase camera only ever copies part of the roll; the marker has to match whatever comes out
    camera.up = v3_normalize(v3(0.3f, 1.0f, 0.0f));
    view_projection = m4_multiply(camera_projection(&camera, WIDTH, HEIGHT), camera_view(&camera));
    check(picking_screen(view_projection, point, (float)WIDTH, (float)HEIGHT, &expected_x, &expected_y),
          "the fixture keeps the target in front of the banked camera");

    mark = locate_target_mark(&camera, point, WIDTH, HEIGHT, MARGIN, MARGIN);
    check(mark.on_screen, "and inside the frame");
    check_close(mark.x, expected_x, 1e-2f, "so the marker lands exactly where picking_screen projects it");
    check_close(mark.y, expected_y, 1e-2f, "in both x and y");
}

void test_hud_main(void)
{
    test_ahead_right();
    test_off_right();
    test_off_left();
    test_behind_right();
    test_behind_left();
    test_banked();
}
