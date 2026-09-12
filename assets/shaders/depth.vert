#version 120

uniform mat4 u_model;
uniform mat4 u_view_projection;

attribute vec3 a_position;
attribute vec2 a_uv;

varying vec2 v_uv;

void main()
{
    v_uv = a_uv;
    gl_Position = u_view_projection * u_model * vec4(a_position, 1.0);
}
