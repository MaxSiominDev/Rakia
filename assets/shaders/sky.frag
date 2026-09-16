#version 120

uniform sampler2D u_panorama;
uniform vec3 u_fog_color;

varying vec3 v_ray;

const float PI = 3.14159265;

void main()
{
    vec3 direction = normalize(v_ray);
    // equirect: the image center looks down -z, u grows toward +x, v from the nadir to the zenith
    float u = 0.5 + atan(direction.x, -direction.z) / (2.0 * PI);
    float v = 0.5 + asin(direction.y) / PI;
    // u2 has the wrap seam a half turn away, so the smaller fwidth of the two corrects the mip bias
    float u2 = fract(u + 0.5);
    float seam_fix = log2(max(min(fwidth(u), fwidth(u2)), 1e-5) / max(fwidth(u), 1e-5));
    // the lowest band fades into the fog the terrain ends in, so the horizon has no seam
    float haze = 1.0 - smoothstep(-0.03, 0.16, direction.y);
    vec3 color = mix(texture2D(u_panorama, vec2(u, v), seam_fix).rgb, u_fog_color, haze);

    gl_FragColor = vec4(color, 1.0);
}
