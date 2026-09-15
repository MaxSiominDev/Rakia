#include "engine/picking.h"

#include <math.h>

// narrows the piece of the ray that stays between one pair of the box's planes; a ray parallel to them runs
// between them for its whole length or never comes near the box at all
static int clip_slab(float origin, float direction, float low, float high, float *near_t, float *far_t)
{
    float first;
    float second;

    if (direction == 0.0f) {
        return origin >= low && origin <= high;
    }

    first = (low - origin) / direction;
    second = (high - origin) / direction;
    *near_t = fmaxf(*near_t, fminf(first, second));
    *far_t = fminf(*far_t, fmaxf(first, second));

    return *near_t <= *far_t;
}

Ray picking_ray(Mat4 view_projection, float x, float y, float width, float height)
{
    const float ndc_x = 2.0f * x / width - 1.0f;
    const float ndc_y = 1.0f - 2.0f * y / height;
    Ray ray = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    Mat4 inverse;

    // a singular view projection leaves nothing to unproject
    if (m4_inverse(view_projection, &inverse) != 0) {
        return ray;
    }

    ray.origin = m4_transform_point(inverse, v3(ndc_x, ndc_y, -1.0f));
    ray.direction = v3_normalize(v3_sub(m4_transform_point(inverse, v3(ndc_x, ndc_y, 1.0f)), ray.origin));

    return ray;
}

int picking_box(Ray ray, Mat4 model, Vec3 min, Vec3 max, float *distance)
{
    Mat4 inverse;
    Vec3 origin;
    Vec3 direction;
    float near_t = -INFINITY;
    float far_t = INFINITY;

    if (m4_inverse(model, &inverse) != 0) {
        return 0;
    }

    origin = m4_transform_point(inverse, ray.origin);
    // left unnormalized: t stays the parameter of the world ray, so the distance comes out in metres
    direction = m4_transform_direction(inverse, ray.direction);
    if (!clip_slab(origin.x, direction.x, min.x, max.x, &near_t, &far_t) ||
        !clip_slab(origin.y, direction.y, min.y, max.y, &near_t, &far_t) ||
        !clip_slab(origin.z, direction.z, min.z, max.z, &near_t, &far_t)) {
        return 0;
    }
    // the box lies entirely behind the start of the ray
    if (far_t < 0.0f) {
        return 0;
    }

    *distance = fmaxf(near_t, 0.0f);

    return 1;
}

int picking_ground(Ray ray, float height, Vec3 *hit)
{
    float t;

    if (fabsf(ray.direction.y) < 1e-6f) {
        return 0;
    }

    t = (height - ray.origin.y) / ray.direction.y;
    if (t < 0.0f) {
        return 0;
    }

    *hit = v3_add(ray.origin, v3_scale(ray.direction, t));

    return 1;
}
