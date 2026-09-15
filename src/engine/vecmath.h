#ifndef VECMATH_H
#define VECMATH_H

#define VEC_PI 3.14159265358979f
#define VEC_DEGREES (VEC_PI / 180.0f)

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec4;

// column-major: element (row, column) lives at m[column * 4 + row], as GL expects
typedef struct {
    float m[16];
} Mat4;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Quat;

float clamped(float value, float low, float high);

Vec3 v3(float x, float y, float z);
Vec3 v3_add(Vec3 a, Vec3 b);
Vec3 v3_sub(Vec3 a, Vec3 b);
Vec3 v3_scale(Vec3 v, float k);
Vec3 v3_lerp(Vec3 a, Vec3 b, float t);
Vec3 v3_cross(Vec3 a, Vec3 b);
float v3_dot(Vec3 a, Vec3 b);
float v3_length(Vec3 v);
Vec3 v3_normalize(Vec3 v);

Vec4 v4(float x, float y, float z, float w);

Mat4 m4_identity(void);
// a * b applies b first, then a, the same way GL composes matrices
Mat4 m4_multiply(Mat4 a, Mat4 b);
Mat4 m4_translate(Vec3 offset);
Mat4 m4_rotate(float radians, Vec3 axis);
Mat4 m4_scale(Vec3 factors);
Mat4 m4_perspective(float fov_y_radians, float aspect, float near_plane, float far_plane);
Mat4 m4_orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);
Mat4 m4_look_at(Vec3 eye, Vec3 target, Vec3 up);
Mat4 m4_transpose(Mat4 m);
// returns -1 and leaves *out untouched when m is singular
int m4_inverse(Mat4 m, Mat4 *out);
Vec4 m4_transform(Mat4 m, Vec4 v);
Vec3 m4_transform_point(Mat4 m, Vec3 point);
Vec3 m4_transform_direction(Mat4 m, Vec3 direction);

Quat quat(float x, float y, float z, float w);
Quat quat_identity(void);
Quat quat_from_axis_angle(Vec3 axis, float radians);
// a * b applies b first, then a
Quat quat_multiply(Quat a, Quat b);
Quat quat_normalize(Quat q);
Mat4 quat_to_mat4(Quat q);
Vec3 quat_rotate(Quat q, Vec3 v);

#endif
