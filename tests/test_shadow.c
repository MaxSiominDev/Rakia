#include "check.h"
#include "engine/shadow.h"
#include "engine/vecmath.h"

void test_shadow_main(void)
{
    const Vec3 center = v3(10.0f, 2.0f, -5.0f);
    const Vec3 sun = v3_normalize(v3(0.0f, 1.0f, 1.0f));
    Shadow shadow;
    Vec3 mapped;

    shadow_fit(&shadow, center, 20.0f, sun);

    check_v3(m4_transform_point(shadow.texture_matrix, center), 0.5f, 0.5f, 0.5f,
             "the center maps to the middle of the map");

    mapped = m4_transform_point(shadow.texture_matrix, v3_add(center, v3_scale(sun, 20.0f)));
    check_close(mapped.z, 0.0f, 1e-5f, "the sphere's sunward edge is at the near plane");
    mapped = m4_transform_point(shadow.texture_matrix, v3_sub(center, v3_scale(sun, 20.0f)));
    check_close(mapped.z, 1.0f, 1e-5f, "the sphere's far edge is at the far plane");

    mapped = m4_transform_point(shadow.texture_matrix, v3_add(center, v3(20.0f, 0.0f, 0.0f)));
    check_close(mapped.x, 1.0f, 1e-5f, "the sphere's +x edge is at the map's right border");
    check_close(mapped.y, 0.5f, 1e-5f, "a sideways offset does not move the vertical map coordinate");

    mapped = m4_transform_point(shadow.view_projection, center);
    check_close(mapped.x, 0.0f, 1e-5f, "clip space is centered");
    check_close(mapped.z, 0.0f, 1e-5f, "clip depth of the center is zero");
}
