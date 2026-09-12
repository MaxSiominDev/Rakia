#ifndef LIGHT_H
#define LIGHT_H

#include "engine/shader.h"
#include "engine/vecmath.h"

typedef struct {
    // unit vector toward the sun
    Vec3 sun_direction;
    // irradiance in the panorama's units: from the sun on a surface facing it, from the sky on one facing up,
    // from the ground on one facing down
    Vec3 sun_color;
    Vec3 sky_ambient;
    Vec3 ground_ambient;
    Vec3 fog_color;
    float fog_density;
    float exposure;
    // turn of the panorama about +y that brings its sun to sun_direction
    float sky_yaw;
} Light;

Light light_golden_hour(void);
void light_apply(const Light *light, Shader *shader);

#endif
