#include "check.h"
#include "engine/picking.h"
#include "engine/vecmath.h"

#include <math.h>

#define FOV (60.0f * VEC_DEGREES)
#define ASPECT (16.0f / 9.0f)
#define NEAR_PLANE 1.0f

static Ray aimed(Vec3 origin, Vec3 direction)
{
    Ray ray;

    ray.origin = origin;
    ray.direction = v3_normalize(direction);

    return ray;
}

static void test_ray(void)
{
    const Vec3 eye = v3(0.0f, 5.0f, -20.0f);
    const Mat4 view_projection = m4_multiply(m4_perspective(FOV, ASPECT, NEAR_PLANE, 500.0f),
                                             m4_look_at(eye, v3(0.0f, 5.0f, 0.0f), v3(0.0f, 1.0f, 0.0f)));
    const Mat4 singular = {{0.0f}};
    const float slope = tanf(FOV * 0.5f);
    const Ray middle = picking_ray(view_projection, 640.0f, 360.0f, 1280.0f, 720.0f);
    const Ray corner = picking_ray(view_projection, 0.0f, 0.0f, 1280.0f, 720.0f);
    const Ray halved = picking_ray(view_projection, 320.0f, 180.0f, 640.0f, 360.0f);
    const Ray nowhere = picking_ray(singular, 640.0f, 360.0f, 1280.0f, 720.0f);

    check_v3(middle.direction, 0.0f, 0.0f, 1.0f, "a ray through the middle of the window runs along the view");
    check_v3(v3_sub(middle.origin, v3_scale(middle.direction, NEAR_PLANE)), 0.0f, 5.0f, -20.0f,
             "and starts a near plane ahead of the eye");

    check_close(corner.direction.y / corner.direction.z, slope, 1e-5f,
                "the top left corner leans up by half the field of view");
    check_close(corner.direction.x / corner.direction.z, slope * ASPECT, 1e-5f,
                "and to the camera's left by as much again, stretched over the aspect");

    check_v3(halved.origin, middle.origin.x, middle.origin.y, middle.origin.z,
             "the middle of a window half the size starts the ray in the same place");
    check_v3(halved.direction, middle.direction.x, middle.direction.y, middle.direction.z,
             "and points it the same way, since the cursor arrives in points either way");

    check_v3(nowhere.origin, 0.0f, 0.0f, 0.0f, "a view projection that cannot be inverted gives a ray at the origin");
    check_v3(nowhere.direction, 0.0f, 0.0f, 1.0f, "pointing along +z");
}

static void test_box(void)
{
    const Vec3 min = v3(-1.0f, -1.0f, -5.0f);
    const Vec3 max = v3(1.0f, 1.0f, 5.0f);
    const Mat4 placed = m4_translate(v3(0.0f, 0.0f, 30.0f));
    const Mat4 turned = m4_multiply(placed, m4_rotate(VEC_PI * 0.5f, v3(0.0f, 1.0f, 0.0f)));
    const Mat4 doubled = m4_multiply(placed, m4_scale(v3(2.0f, 2.0f, 2.0f)));
    const Ray ahead = aimed(v3(0.0f, 0.0f, -20.0f), v3(0.0f, 0.0f, 1.0f));
    const Ray aside = aimed(v3(-30.0f, 0.0f, 33.0f), v3(1.0f, 0.0f, 0.0f));
    float distance;

    check(picking_box(ahead, placed, min, max, &distance) == 1, "a ray down the middle hits the crate");
    check_close(distance, 45.0f, 1e-4f, "at the near face, not the far one");

    check(picking_box(ahead, turned, min, max, &distance) == 1, "the crate turned across the ray is hit too");
    check_close(distance, 49.0f, 1e-4f, "now on its short side, four metres further out");

    check(picking_box(ahead, doubled, min, max, &distance) == 1, "a crate built twice the size is hit");
    check_close(distance, 40.0f, 1e-4f, "as far out as its own bounds grew, in metres");

    check(picking_box(aside, placed, min, max, &distance) == 1, "a ray across the far end of the crate hits it");
    check(picking_box(aside, turned, min, max, &distance) == 0, "and misses once the crate is turned out of its way");

    check(picking_box(aimed(v3(3.0f, 0.0f, -20.0f), v3(0.0f, 0.0f, 1.0f)), placed, min, max, &distance) == 0,
          "a ray running past the side of the crate misses");
    check(picking_box(aimed(v3(0.0f, 0.0f, -20.0f), v3(0.0f, 0.0f, -1.0f)), placed, min, max, &distance) == 0,
          "so does one that leaves the crate behind it");

    check(picking_box(aimed(v3(0.0f, 0.0f, 30.0f), v3(0.0f, 0.0f, 1.0f)), placed, min, max, &distance) == 1,
          "a ray that starts inside the crate hits it");
    check_close(distance, 0.0f, 1e-6f, "with nothing to travel first");
}

static void test_ground(void)
{
    const Ray down = aimed(v3(0.0f, 100.0f, 0.0f), v3(0.0f, -1.0f, 1.0f));
    Vec3 hit;

    check(picking_ground(down, 20.0f, &hit) == 1, "a ray leaning down reaches the plane");
    check_v3(hit, 0.0f, 20.0f, 80.0f, "eighty metres out, where it drops to that height");

    check(picking_ground(down, 200.0f, &hit) == 0, "a plane above the ray is behind it and misses");
    check(picking_ground(aimed(v3(0.0f, 100.0f, 0.0f), v3(0.0f, 1.0f, 0.0f)), 20.0f, &hit) == 0,
          "a ray pointing up never comes down to it");
    check(picking_ground(aimed(v3(0.0f, 100.0f, 0.0f), v3(0.0f, 0.0f, 1.0f)), 20.0f, &hit) == 0,
          "and one running level with it never meets it");
}

void test_picking_main(void)
{
    test_ray();
    test_box();
    test_ground();
}
