#version 120

uniform mat4 u_view_projection;
uniform mat4 u_shadow_matrix;
// the point the card turns toward: the camera when shading, the sun when filling the shadow map
uniform vec3 u_view_origin;
uniform vec3 u_camera_position;
uniform float u_fade_end;
uniform float u_fade_per_metre;

attribute vec3 a_position;
// corner of the quad in x and y, half width and height of the plant in z and w
attribute vec4 a_corner;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_shadow_coord;

void main()
{
    vec3 to_view = u_view_origin - a_position;
    vec3 facing = normalize(vec3(to_view.x, 0.0, to_view.z));
    vec3 right = vec3(facing.z, 0.0, -facing.x);
    float side = a_corner.x - 0.5;
    // a plant drops out once it is about two pixels tall, which is much further for a tree than for a tuft
    float reach = min(u_fade_end, a_corner.w * u_fade_per_metre);
    float fade = 1.0 - smoothstep(reach * 0.85, reach, distance(u_camera_position, a_position));
    vec4 world = vec4(a_position + right * side * 2.0 * a_corner.z * fade, 1.0);

    world.y += a_corner.y * a_corner.w * fade;
    // leaves point every way, so the card is shaded as a rounded bush instead of a flat sign
    v_normal = normalize(mix(facing, vec3(0.0, 1.0, 0.0), 0.5) + right * side * 0.7);
    v_world_position = world.xyz;
    v_uv = a_corner.xy;
    v_shadow_coord = u_shadow_matrix * world;
    gl_Position = u_view_projection * world;
}
