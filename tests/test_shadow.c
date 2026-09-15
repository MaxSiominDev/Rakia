#include "check.h"
#include "engine/shadow.h"
#include "engine/vecmath.h"

static void test_fit(void)
{
    const Vec3 center = v3(10.0f, 2.0f, -5.0f);
    const Vec3 sun = v3_normalize(v3(0.0f, 1.0f, 1.0f));
    const float radius = 20.0f;
    const float relief = 20.0f;
    const float reach = 20.0f;
    Shadow shadow;
    Vec3 mapped;

    shadow_fit(&shadow, center, radius, relief, reach, sun);

    check_v3(m4_transform_point(shadow.texture_matrix, center), 0.5f, 0.5f, 0.5f,
             "the center maps to the middle of the map");

    mapped = m4_transform_point(shadow.texture_matrix, v3_add(center, v3(radius, 0.0f, 0.0f)));
    check_close(mapped.x, 1.0f, 1e-5f, "the ground reaches the radius across the light");
    check_close(mapped.y, 0.5f, 1e-5f, "without moving along it");

    // the caster the fit is made for stands reach above the ground, which is that far up the sun ray
    mapped = m4_transform_point(shadow.texture_matrix, v3_add(center, v3_scale(sun, reach / sun.y)));
    check_close(mapped.x, 0.5f, 1e-5f, "a caster over the center keeps the middle across the light");
    check_close(mapped.y, 0.5f, 1e-5f, "and along it, since it shades that very spot");
    check(mapped.z > 0.0f && mapped.z < 0.5f, "while sitting between the near plane and the ground");

    mapped = m4_transform_point(shadow.texture_matrix, v3_add(center, v3(0.0f, 0.0f, radius)));
    check(mapped.y > 0.0f && mapped.y < 0.5f, "ground toward the sun leans down the map");
    mapped = m4_transform_point(shadow.texture_matrix, v3_sub(center, v3(0.0f, 0.0f, radius)));
    check(mapped.y > 0.5f && mapped.y < 1.0f, "and ground away from it up");

    mapped = m4_transform_point(shadow.texture_matrix, v3_add(center, v3(0.0f, relief, 0.0f)));
    check(mapped.y > 0.5f && mapped.y < 1.0f, "a hill as tall as the relief still lands on the map");

    mapped = m4_transform_point(shadow.view_projection, center);
    check_close(mapped.x, 0.0f, 1e-5f, "clip space is centered");
    check_close(mapped.z, 0.0f, 1e-5f, "clip depth of the center is zero");
}

// the fit the flight uses: a sun ten degrees up, a wide piece of ground and the jet high above it
static void test_low_sun(void)
{
    const Vec3 center = v3(0.0f, 0.0f, 0.0f);
    const Vec3 sun = v3_normalize(v3(0.0f, 0.1736f, -0.9848f));
    const float radius = 900.0f;
    const float relief = 200.0f;
    const float reach = 150.0f;
    Shadow shadow;
    Vec3 mapped;

    shadow_fit(&shadow, center, radius, relief, reach, sun);

    mapped = m4_transform_point(shadow.texture_matrix, v3_scale(sun, reach / sun.y));
    check_close(mapped.x, 0.5f, 1e-4f, "the jet lands on its own shadow across the light");
    check_close(mapped.y, 0.5f, 1e-4f, "and along it");
    check(mapped.z > 0.0f && mapped.z < 0.5f, "well inside the near plane");

    mapped = m4_transform_point(shadow.texture_matrix, v3(0.0f, relief, 0.0f));
    check(mapped.y > 0.5f && mapped.y < 1.0f, "a hill of the stated relief is on the map");
    mapped = m4_transform_point(shadow.texture_matrix, v3(0.0f, 3.0f * relief, 0.0f));
    check(mapped.y > 1.0f, "and one three times as tall is not, since the fit is no taller than it was told");

    // the ground leans along the light by the sine of the elevation only, which is why that axis is fitted
    // so much tighter than the one across it
    mapped = m4_transform_point(shadow.texture_matrix, v3(radius, 0.0f, 0.0f));
    check_close(mapped.x, 0.0f, 1e-4f, "a whole radius across the light lands on the edge of the map");
    mapped = m4_transform_point(shadow.texture_matrix, v3(0.0f, 0.0f, radius));
    check(mapped.y > 0.65f && mapped.y < 0.8f, "while the same distance along it climbs a fifth of the way up");
}

void test_shadow_main(void)
{
    test_fit();
    test_low_sun();
}
