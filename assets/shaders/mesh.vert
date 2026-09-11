#version 120

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;

void main()
{
    vec4 world = u_model * vec4(a_position, 1.0);

    v_world_position = world.xyz;
    // models are placed with rotation and uniform scale only, so the model matrix rotates normals correctly
    v_normal = mat3(u_model) * a_normal;
    v_uv = a_uv;
    gl_Position = u_projection * u_view * world;
}
