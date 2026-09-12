#version 120

uniform mat4 u_model;
uniform mat4 u_view_projection;
uniform mat4 u_shadow_matrix;

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;
attribute vec4 a_tangent;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec4 v_tangent;
varying vec2 v_uv;
varying vec4 v_shadow_coord;

void main()
{
    vec4 world = u_model * vec4(a_position, 1.0);

    v_world_position = world.xyz;
    // entities are placed with rotation and uniform scale only, so the model matrix turns normals correctly
    v_normal = mat3(u_model) * a_normal;
    v_tangent = vec4(mat3(u_model) * a_tangent.xyz, a_tangent.w);
    v_uv = a_uv;
    v_shadow_coord = u_shadow_matrix * world;
    gl_Position = u_view_projection * world;
}
