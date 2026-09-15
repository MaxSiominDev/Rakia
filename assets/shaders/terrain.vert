#version 120

uniform mat4 u_view_projection;
uniform mat4 u_shadow_matrix;

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_uv;
attribute vec4 a_tangent;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec4 v_tangent;
varying vec2 v_blend;
varying vec4 v_shadow_coord;

void main()
{
    // chunks are built in world space, so there is no model matrix to apply
    v_world_position = a_position;
    v_normal = a_normal;
    v_tangent = a_tangent;
    // the chunk carries the biome weights in the uv slot and tiles its textures by world position
    v_blend = a_uv;
    v_shadow_coord = u_shadow_matrix * vec4(a_position, 1.0);
    gl_Position = u_view_projection * vec4(a_position, 1.0);
}
