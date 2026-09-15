#include "engine/vecmath.h"

#include <math.h>

float clamped(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}

Vec3 v3(float x, float y, float z)
{
    Vec3 result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

Vec3 v3_add(Vec3 a, Vec3 b)
{
    return v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 v3_sub(Vec3 a, Vec3 b)
{
    return v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 v3_scale(Vec3 v, float k)
{
    return v3(v.x * k, v.y * k, v.z * k);
}

Vec3 v3_lerp(Vec3 a, Vec3 b, float t)
{
    return v3_add(a, v3_scale(v3_sub(b, a), t));
}

Vec3 v3_cross(Vec3 a, Vec3 b)
{
    return v3(a.y * b.z - a.z * b.y,
              a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}

float v3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float v3_length(Vec3 v)
{
    return sqrtf(v3_dot(v, v));
}

Vec3 v3_normalize(Vec3 v)
{
    const float length = v3_length(v);

    return length > 0.0f ? v3_scale(v, 1.0f / length) : v;
}

Vec4 v4(float x, float y, float z, float w)
{
    Vec4 result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

Mat4 m4_identity(void)
{
    Mat4 result = {{0.0f}};

    result.m[0] = 1.0f;
    result.m[5] = 1.0f;
    result.m[10] = 1.0f;
    result.m[15] = 1.0f;

    return result;
}

Mat4 m4_multiply(Mat4 a, Mat4 b)
{
    Mat4 result;
    int column;
    int row;

    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            result.m[column * 4 + row] = a.m[row] * b.m[column * 4] +
                                         a.m[4 + row] * b.m[column * 4 + 1] +
                                         a.m[8 + row] * b.m[column * 4 + 2] +
                                         a.m[12 + row] * b.m[column * 4 + 3];
        }
    }

    return result;
}

Mat4 m4_translate(Vec3 offset)
{
    Mat4 result = m4_identity();

    result.m[12] = offset.x;
    result.m[13] = offset.y;
    result.m[14] = offset.z;

    return result;
}

Mat4 m4_rotate(float radians, Vec3 axis)
{
    const Vec3 n = v3_normalize(axis);
    const float c = cosf(radians);
    const float s = sinf(radians);
    const float t = 1.0f - c;
    Mat4 result = m4_identity();

    result.m[0] = t * n.x * n.x + c;
    result.m[1] = t * n.x * n.y + s * n.z;
    result.m[2] = t * n.x * n.z - s * n.y;
    result.m[4] = t * n.x * n.y - s * n.z;
    result.m[5] = t * n.y * n.y + c;
    result.m[6] = t * n.y * n.z + s * n.x;
    result.m[8] = t * n.x * n.z + s * n.y;
    result.m[9] = t * n.y * n.z - s * n.x;
    result.m[10] = t * n.z * n.z + c;

    return result;
}

Mat4 m4_scale(Vec3 factors)
{
    Mat4 result = m4_identity();

    result.m[0] = factors.x;
    result.m[5] = factors.y;
    result.m[10] = factors.z;

    return result;
}

Mat4 m4_perspective(float fov_y_radians, float aspect, float near_plane, float far_plane)
{
    const float f = 1.0f / tanf(fov_y_radians * 0.5f);
    const float depth = near_plane - far_plane;
    Mat4 result = {{0.0f}};

    result.m[0] = f / aspect;
    result.m[5] = f;
    result.m[10] = (far_plane + near_plane) / depth;
    result.m[11] = -1.0f;
    result.m[14] = 2.0f * far_plane * near_plane / depth;

    return result;
}

Mat4 m4_orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
{
    Mat4 result = m4_identity();

    result.m[0] = 2.0f / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[10] = -2.0f / (far_plane - near_plane);
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -(far_plane + near_plane) / (far_plane - near_plane);

    return result;
}

Mat4 m4_look_at(Vec3 eye, Vec3 target, Vec3 up)
{
    const Vec3 forward = v3_normalize(v3_sub(target, eye));
    const Vec3 side = v3_normalize(v3_cross(forward, up));
    const Vec3 camera_up = v3_cross(side, forward);
    Mat4 result = m4_identity();

    result.m[0] = side.x;
    result.m[4] = side.y;
    result.m[8] = side.z;
    result.m[1] = camera_up.x;
    result.m[5] = camera_up.y;
    result.m[9] = camera_up.z;
    result.m[2] = -forward.x;
    result.m[6] = -forward.y;
    result.m[10] = -forward.z;
    result.m[12] = -v3_dot(side, eye);
    result.m[13] = -v3_dot(camera_up, eye);
    result.m[14] = v3_dot(forward, eye);

    return result;
}

Mat4 m4_transpose(Mat4 m)
{
    Mat4 result;
    int column;
    int row;

    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            result.m[column * 4 + row] = m.m[row * 4 + column];
        }
    }

    return result;
}

int m4_inverse(Mat4 m, Mat4 *out)
{
    const float *a = m.m;
    float inv[16];
    float det;
    int i;

    inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] +
             a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
    inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] -
             a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
    inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] +
             a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
    inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] -
              a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
    inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] -
             a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
    inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] +
             a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
    inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] -
             a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
    inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] +
              a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
    inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] +
             a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
    inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] -
             a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
    inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] +
              a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
    inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] -
              a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
    inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] -
             a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
    inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] +
             a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
    inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] -
              a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
    inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] +
              a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];

    det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
    if (fabsf(det) < 1e-12f) {
        return -1;
    }

    for (i = 0; i < 16; i++) {
        out->m[i] = inv[i] / det;
    }

    return 0;
}

Vec4 m4_transform(Mat4 m, Vec4 v)
{
    return v4(m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w,
              m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w,
              m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w,
              m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w);
}

Vec3 m4_transform_point(Mat4 m, Vec3 point)
{
    const Vec4 h = m4_transform(m, v4(point.x, point.y, point.z, 1.0f));
    const float scale = h.w != 0.0f ? 1.0f / h.w : 1.0f;

    return v3(h.x * scale, h.y * scale, h.z * scale);
}

Vec3 m4_transform_direction(Mat4 m, Vec3 direction)
{
    const Vec4 h = m4_transform(m, v4(direction.x, direction.y, direction.z, 0.0f));

    return v3(h.x, h.y, h.z);
}

Quat quat(float x, float y, float z, float w)
{
    Quat result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

Quat quat_identity(void)
{
    return quat(0.0f, 0.0f, 0.0f, 1.0f);
}

Quat quat_from_axis_angle(Vec3 axis, float radians)
{
    const Vec3 n = v3_normalize(axis);
    const float s = sinf(radians * 0.5f);

    return quat(n.x * s, n.y * s, n.z * s, cosf(radians * 0.5f));
}

Quat quat_multiply(Quat a, Quat b)
{
    return quat(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

Quat quat_normalize(Quat q)
{
    const float length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);

    return length > 0.0f ? quat(q.x / length, q.y / length, q.z / length, q.w / length) : quat_identity();
}

Mat4 quat_to_mat4(Quat q)
{
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float wx = q.w * q.x;
    const float wy = q.w * q.y;
    const float wz = q.w * q.z;
    Mat4 result = m4_identity();

    result.m[0] = 1.0f - 2.0f * (yy + zz);
    result.m[1] = 2.0f * (xy + wz);
    result.m[2] = 2.0f * (xz - wy);
    result.m[4] = 2.0f * (xy - wz);
    result.m[5] = 1.0f - 2.0f * (xx + zz);
    result.m[6] = 2.0f * (yz + wx);
    result.m[8] = 2.0f * (xz + wy);
    result.m[9] = 2.0f * (yz - wx);
    result.m[10] = 1.0f - 2.0f * (xx + yy);

    return result;
}

Vec3 quat_rotate(Quat q, Vec3 v)
{
    const Vec3 axis = v3(q.x, q.y, q.z);
    const Vec3 t = v3_scale(v3_cross(axis, v), 2.0f);

    return v3_add(v3_add(v, v3_scale(t, q.w)), v3_cross(axis, t));
}
