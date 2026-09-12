#version 120

uniform mat3 u_ray_basis;

attribute vec3 a_position;

varying vec3 v_ray;

void main()
{
    // the ray is linear in screen position; interpolating it from the corners gives the exact per-pixel view ray
    v_ray = u_ray_basis * vec3(a_position.xy, 1.0);
    gl_Position = vec4(a_position.xy, 0.0, 1.0);
}
