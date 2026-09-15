#version 120

uniform mat4 u_view_projection;
uniform vec3 u_view_origin;
uniform vec3 u_camera_position;
uniform float u_fade_end;
uniform float u_fade_per_metre;

attribute vec3 a_position;
attribute vec4 a_corner;

varying vec2 v_uv;

void main()
{
    vec3 to_view = u_view_origin - a_position;
    vec3 facing = normalize(vec3(to_view.x, 0.0, to_view.z));
    vec3 right = vec3(facing.z, 0.0, -facing.x);
    // the same fade as the lit pass, or a plant would keep a shadow after it has shrunk away
    float reach = min(u_fade_end, a_corner.w * u_fade_per_metre);
    float fade = 1.0 - smoothstep(reach * 0.85, reach, distance(u_camera_position, a_position));
    vec4 world = vec4(a_position + right * (a_corner.x - 0.5) * 2.0 * a_corner.z * fade, 1.0);

    world.y += a_corner.y * a_corner.w * fade;
    v_uv = a_corner.xy;
    gl_Position = u_view_projection * world;
}
