#version 120

uniform mat4 u_view_projection;
// the camera's own axes, so every sprite stands square to the eye
uniform vec3 u_right;
uniform vec3 u_up;

attribute vec3 a_position;
attribute vec2 a_uv;
// corner of the sprite in x and y, its size in z and its turn in w
attribute vec4 a_corner;
attribute vec4 a_color;

varying vec2 v_uv;
varying vec4 v_color;
varying vec3 v_world_position;

void main()
{
    float sine = sin(a_corner.w);
    float cosine = cos(a_corner.w);
    vec2 corner = vec2(a_corner.x * cosine - a_corner.y * sine,
                       a_corner.x * sine + a_corner.y * cosine) * a_corner.z;
    vec4 world = vec4(a_position + u_right * corner.x + u_up * corner.y, 1.0);

    v_uv = a_uv;
    v_color = a_color;
    v_world_position = world.xyz;
    gl_Position = u_view_projection * world;
}
