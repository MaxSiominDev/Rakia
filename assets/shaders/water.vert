#version 120

uniform mat4 u_model;
uniform mat4 u_view_projection;

attribute vec3 a_position;

varying vec3 v_world_position;

void main()
{
    vec4 world = u_model * vec4(a_position, 1.0);

    v_world_position = world.xyz;
    gl_Position = u_view_projection * world;
}
