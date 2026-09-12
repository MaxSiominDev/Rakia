#include "check.h"
#include "engine/vecmath.h"

#include <math.h>

#define HALF_PI (VEC_PI * 0.5f)

static void check_m4(Mat4 actual, Mat4 expected, const char *what)
{
    int i;

    for (i = 0; i < 16; i++) {
        check_close(actual.m[i], expected.m[i], 1e-5f, what);
    }
}

static void test_vectors(void)
{
    check_v3(v3_cross(v3(1.0f, 0.0f, 0.0f), v3(0.0f, 1.0f, 0.0f)), 0.0f, 0.0f, 1.0f, "x cross y is z");
    check_close(v3_length(v3(3.0f, 4.0f, 12.0f)), 13.0f, 1e-5f, "length");
    check_close(v3_length(v3_normalize(v3(3.0f, 4.0f, 12.0f))), 1.0f, 1e-6f, "normalized length");
    check_v3(v3_normalize(v3(0.0f, 0.0f, 0.0f)), 0.0f, 0.0f, 0.0f, "normalizing zero stays zero");
    check_v3(v3_lerp(v3(0.0f, 0.0f, 0.0f), v3(2.0f, 4.0f, 6.0f), 0.5f), 1.0f, 2.0f, 3.0f, "lerp midpoint");
    check_close(v3_dot(v3(1.0f, 2.0f, 3.0f), v3(4.0f, 5.0f, 6.0f)), 32.0f, 1e-6f, "dot");
}

static void test_products(void)
{
    const Mat4 translate = m4_translate(v3(1.0f, 0.0f, 0.0f));
    const Mat4 rotate = m4_rotate(HALF_PI, v3(0.0f, 0.0f, 1.0f));
    const Mat4 scale = m4_scale(v3(2.0f, 3.0f, 4.0f));

    check_m4(m4_multiply(m4_identity(), scale), scale, "identity times a matrix");
    check_m4(m4_multiply(scale, m4_identity()), scale, "a matrix times identity");
    check_v3(m4_transform_point(m4_multiply(translate, rotate), v3(1.0f, 0.0f, 0.0f)),
             1.0f, 1.0f, 0.0f, "translate * rotate rotates first");
    check_v3(m4_transform_point(m4_multiply(rotate, translate), v3(1.0f, 0.0f, 0.0f)),
             0.0f, 2.0f, 0.0f, "rotate * translate translates first");
    check_v3(m4_transform_point(scale, v3(1.0f, 1.0f, 1.0f)), 2.0f, 3.0f, 4.0f, "scale");
    check_v3(m4_transform_direction(translate, v3(0.0f, 0.0f, 1.0f)), 0.0f, 0.0f, 1.0f,
             "directions ignore translation");
    check_v3(m4_transform_point(m4_rotate(HALF_PI, v3(0.0f, 2.0f, 0.0f)), v3(1.0f, 0.0f, 0.0f)),
             0.0f, 0.0f, -1.0f, "rotation normalizes its axis");
}

static void test_transpose_and_inverse(void)
{
    const Mat4 model = m4_multiply(m4_translate(v3(1.0f, -2.0f, 3.0f)),
                                   m4_multiply(m4_rotate(0.7f, v3(1.0f, 2.0f, 3.0f)),
                                               m4_scale(v3(2.0f, 0.5f, 1.5f))));
    const Mat4 translate = m4_translate(v3(1.0f, 2.0f, 3.0f));
    Mat4 inverse;
    Mat4 ignored;

    check_close(m4_transpose(translate).m[3], 1.0f, 1e-6f, "transpose moves the translation row");
    check_m4(m4_transpose(m4_transpose(model)), model, "transpose twice is identity");

    check(m4_inverse(model, &inverse) == 0, "affine matrix is invertible");
    check_m4(m4_multiply(model, inverse), m4_identity(), "matrix times inverse");
    check_m4(m4_multiply(inverse, model), m4_identity(), "inverse times matrix");
    check_v3(m4_transform_point(inverse, m4_transform_point(model, v3(0.3f, -0.4f, 0.5f))),
             0.3f, -0.4f, 0.5f, "inverse round trip on a point");
    check(m4_inverse(m4_scale(v3(1.0f, 0.0f, 1.0f)), &ignored) == -1, "singular matrix is rejected");
}

static void test_perspective(void)
{
    const Mat4 p = m4_perspective(HALF_PI, 2.0f, 1.0f, 3.0f);

    check_close(p.m[0], 0.5f, 1e-6f, "perspective x scale");
    check_close(p.m[5], 1.0f, 1e-6f, "perspective y scale");
    check_close(p.m[10], -2.0f, 1e-6f, "perspective depth scale");
    check_close(p.m[11], -1.0f, 1e-6f, "perspective w row");
    check_close(p.m[14], -3.0f, 1e-6f, "perspective depth offset");
    check_close(p.m[15], 0.0f, 1e-6f, "perspective has no constant w");
    check_close(m4_transform_point(p, v3(0.0f, 0.0f, -1.0f)).z, -1.0f, 1e-5f, "near plane maps to -1");
    check_close(m4_transform_point(p, v3(0.0f, 0.0f, -3.0f)).z, 1.0f, 1e-5f, "far plane maps to +1");
    check_close(m4_transform_point(p, v3(2.0f, 0.0f, -1.0f)).x, 1.0f, 1e-5f, "right edge maps to +1");
}

static void test_orthographic(void)
{
    const Mat4 o = m4_orthographic(-2.0f, 2.0f, -1.0f, 1.0f, 0.0f, 10.0f);

    check_v3(m4_transform_point(o, v3(2.0f, 1.0f, -10.0f)), 1.0f, 1.0f, 1.0f, "orthographic far corner");
    check_v3(m4_transform_point(o, v3(-2.0f, -1.0f, 0.0f)), -1.0f, -1.0f, -1.0f, "orthographic near corner");
}

static void test_look_at(void)
{
    const Mat4 from_z = m4_look_at(v3(0.0f, 0.0f, 5.0f), v3(0.0f, 0.0f, 0.0f), v3(0.0f, 1.0f, 0.0f));
    const Mat4 from_x = m4_look_at(v3(5.0f, 0.0f, 0.0f), v3(0.0f, 0.0f, 0.0f), v3(0.0f, 1.0f, 0.0f));

    check_m4(from_z, m4_translate(v3(0.0f, 0.0f, -5.0f)), "looking down -z is a translation");
    check_v3(m4_transform_point(from_z, v3(1.0f, 2.0f, 0.0f)), 1.0f, 2.0f, -5.0f, "point in front from +z");
    check_v3(m4_transform_point(from_x, v3(0.0f, 0.0f, 0.0f)), 0.0f, 0.0f, -5.0f, "target lands on -z");
    check_v3(m4_transform_point(from_x, v3(0.0f, 0.0f, -1.0f)), 1.0f, 0.0f, -5.0f, "world -z is to the right");
    check_v3(m4_transform_point(from_x, v3(0.0f, 1.0f, 0.0f)), 0.0f, 1.0f, -5.0f, "world up stays up");
}

static void test_quaternions(void)
{
    const Quat about_y = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), HALF_PI);
    const Quat about_z = quat_from_axis_angle(v3(0.0f, 0.0f, 1.0f), HALF_PI);
    const Quat about_x = quat_from_axis_angle(v3(1.0f, 0.0f, 0.0f), HALF_PI);
    const Vec3 tilted = v3(1.0f, 2.0f, 3.0f);
    const Quat tilted_quat = quat_from_axis_angle(tilted, 0.7f);
    const Quat scaled = quat(0.0f, 0.0f, 3.0f, 4.0f);
    const Quat unit = quat_normalize(scaled);
    const Vec3 point = v3(0.2f, -0.5f, 0.9f);
    const Vec3 by_matrix = m4_transform_point(m4_rotate(0.7f, tilted), point);

    check_v3(quat_rotate(quat_identity(), tilted), 1.0f, 2.0f, 3.0f, "identity rotation");
    check_v3(quat_rotate(about_y, v3(1.0f, 0.0f, 0.0f)), 0.0f, 0.0f, -1.0f, "90 degrees about y");
    check_v3(quat_rotate(about_z, v3(1.0f, 0.0f, 0.0f)), 0.0f, 1.0f, 0.0f, "90 degrees about z");
    check_v3(quat_rotate(quat_multiply(about_z, about_z), v3(1.0f, 0.0f, 0.0f)), -1.0f, 0.0f, 0.0f,
             "two quarter turns make a half turn");
    check_v3(quat_rotate(quat_multiply(about_z, about_x), v3(0.0f, 1.0f, 0.0f)), 0.0f, 0.0f, 1.0f,
             "a * b applies b first");
    check_m4(quat_to_mat4(tilted_quat), m4_rotate(0.7f, tilted), "quaternion matrix matches the axis-angle matrix");
    check_m4(quat_to_mat4(quat_multiply(about_z, about_x)),
             m4_multiply(quat_to_mat4(about_z), quat_to_mat4(about_x)),
             "quaternion product matches the matrix product");
    check_v3(quat_rotate(tilted_quat, point), by_matrix.x, by_matrix.y, by_matrix.z,
             "quaternion rotation matches the matrix rotation");
    check_close(sqrtf(unit.x * unit.x + unit.y * unit.y + unit.z * unit.z + unit.w * unit.w), 1.0f, 1e-6f,
                "normalized quaternion has unit length");
    check_close(unit.z, 0.6f, 1e-6f, "normalize keeps the direction");
    check_close(quat_normalize(quat(0.0f, 0.0f, 0.0f, 0.0f)).w, 1.0f, 1e-6f, "normalizing zero gives identity");
}

void test_vecmath_main(void)
{
    test_vectors();
    test_products();
    test_transpose_and_inverse();
    test_perspective();
    test_orthographic();
    test_look_at();
    test_quaternions();
}
