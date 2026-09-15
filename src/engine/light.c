#include "engine/light.h"

#include <math.h>

// measured on belfast_sunset_puresky_2k.hdr: the sun disk sits at azimuth 37.8 degrees (from -z toward +x) and
// elevation 2.7; the light is raised to 10 so the wings and the apron still catch it, the glow around the disk
// hides the shift
#define SUN_AZIMUTH_DEGREES 37.8f
#define SUN_ELEVATION_DEGREES 10.0f
// the sun over the hero camera's right shoulder: it lights the jet's left flank and its glow fills the top right
#define SKY_YAW_DEGREES -13.0f
#define EXPOSURE 0.5f
// exp-squared fog: at this distance the haze is about two thirds of the way to the fog color
#define FOG_DISTANCE 7000.0f

// the disk is clipped in the file, so only its color (32.6:20.1:8.8) is measured and its strength chosen by eye;
// the hemisphere values are the irradiance integrated from the file, the fog color its horizon band
static const Vec3 sun_color = {22.0f, 13.6f, 5.9f};
static const Vec3 sky_ambient = {3.37f, 3.29f, 4.65f};
static const Vec3 ground_ambient = {1.54f, 1.43f, 1.90f};
static const Vec3 fog_color = {0.51f, 0.44f, 0.50f};

Light light_golden_hour(void)
{
    const float azimuth = SUN_AZIMUTH_DEGREES * VEC_DEGREES;
    const float elevation = SUN_ELEVATION_DEGREES * VEC_DEGREES;
    const Vec3 in_panorama = v3(cosf(elevation) * sinf(azimuth), sinf(elevation), -cosf(elevation) * cosf(azimuth));
    Light light;

    light.sky_yaw = SKY_YAW_DEGREES * VEC_DEGREES;
    light.sun_direction = m4_transform_direction(m4_rotate(-light.sky_yaw, v3(0.0f, 1.0f, 0.0f)), in_panorama);
    light.sun_color = sun_color;
    light.sky_ambient = sky_ambient;
    light.ground_ambient = ground_ambient;
    light.fog_color = fog_color;
    light.fog_density = 1.0f / FOG_DISTANCE;
    light.exposure = EXPOSURE;

    return light;
}

void light_apply(const Light *light, Shader *shader)
{
    shader_set_vec3(shader, "u_sun_direction", light->sun_direction);
    shader_set_vec3(shader, "u_sun_color", light->sun_color);
    shader_set_vec3(shader, "u_sky_ambient", light->sky_ambient);
    shader_set_vec3(shader, "u_ground_ambient", light->ground_ambient);
    shader_set_vec3(shader, "u_fog_color", light->fog_color);
    shader_set_float(shader, "u_fog_density", light->fog_density);
    shader_set_float(shader, "u_exposure", light->exposure);
}
