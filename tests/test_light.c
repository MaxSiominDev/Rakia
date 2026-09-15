#include "check.h"
#include "engine/light.h"
#include "engine/vecmath.h"

#include <math.h>

// the exp-squared curve the shaders use
static float haze(float distance, float density)
{
    const float d = distance * density;

    return 1.0f - expf(-d * d);
}

void test_light_main(void)
{
    const Light light = light_golden_hour();
    // the sky shader turns a world direction by sky_yaw before the panorama lookup; the same turn must land
    // the sun where the file has it
    const Vec3 in_panorama = m4_transform_direction(m4_rotate(light.sky_yaw, v3(0.0f, 1.0f, 0.0f)),
                                                    light.sun_direction);

    check_close(v3_length(light.sun_direction), 1.0f, 1e-5f, "sun direction is unit");
    check(light.sun_direction.y > 0.0f, "sun is above the horizon");
    check_close(atan2f(in_panorama.x, -in_panorama.z) / VEC_DEGREES, 37.8f, 0.01f, "sun azimuth in the panorama");
    check_close(asinf(in_panorama.y) / VEC_DEGREES, 10.0f, 0.01f, "sun elevation in the panorama");
    check(light.sun_color.x > light.sun_color.y && light.sun_color.y > light.sun_color.z, "sun is warm");
    check(light.sky_ambient.z > light.sky_ambient.x, "sky ambient is bluish");
    check(light.ground_ambient.y < light.sky_ambient.y, "ground ambient is darker than the sky");
    check(haze(500.0f, light.fog_density) < 0.02f, "500 m is nearly clear");
    check(haze(8000.0f, light.fog_density) > 0.7f, "the view distance is mostly haze");
}
