#include "engine/collision.h"

#include <math.h>

// the box's own axes in the ground plane, which are +x and +z of the model after the yaw
static void footprint_axes(const Footprint *box, Vec3 *along_x, Vec3 *along_z)
{
    const float sine = sinf(box->yaw);
    const float cosine = cosf(box->yaw);

    *along_x = v3(cosine, 0.0f, -sine);
    *along_z = v3(sine, 0.0f, cosine);
}

static float footprint_reach(const Footprint *box, Vec3 axis)
{
    Vec3 along_x;
    Vec3 along_z;

    footprint_axes(box, &along_x, &along_z);

    return box->half_x * fabsf(v3_dot(along_x, axis)) + box->half_z * fabsf(v3_dot(along_z, axis));
}

Footprint collision_footprint(Vec3 position, float yaw, Vec3 min, Vec3 max)
{
    const Quat turn = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), yaw);
    const Vec3 center = v3_scale(v3_add(min, max), 0.5f);
    Footprint result;

    result.center = v3_add(position, quat_rotate(turn, center));
    result.half_x = (max.x - min.x) * 0.5f;
    result.half_z = (max.z - min.z) * 0.5f;
    result.yaw = yaw;

    return result;
}

int collision_push(const Footprint *a, const Footprint *b, Vec3 *push)
{
    const Vec3 offset = v3_sub(a->center, b->center);
    Vec3 axes[4];
    float overlap[4];
    int closest = 0;
    int i;

    footprint_axes(a, &axes[0], &axes[1]);
    footprint_axes(b, &axes[2], &axes[3]);

    for (i = 0; i < 4; i++) {
        overlap[i] = footprint_reach(a, axes[i]) + footprint_reach(b, axes[i]) - fabsf(v3_dot(offset, axes[i]));
        // a gap along any of the four axes keeps the boxes apart
        if (overlap[i] <= 0.0f) {
            return 0;
        }
        if (overlap[i] < overlap[closest]) {
            closest = i;
        }
    }

    // the axis points either way, so the side a stands on decides which way the move goes
    if (v3_dot(offset, axes[closest]) < 0.0f) {
        overlap[closest] = -overlap[closest];
    }
    *push = v3_scale(axes[closest], overlap[closest]);

    return 1;
}
