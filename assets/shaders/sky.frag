#version 120

uniform sampler2D u_panorama;
uniform vec3 u_fog_color;
uniform float u_exposure;

varying vec3 v_ray;

const float PI = 3.14159265;

// fitted ACES curve by Krzysztof Narkowicz, the same as in mesh.frag so sky and objects match
vec3 tonemap(vec3 color)
{
    vec3 mapped = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);

    return pow(clamp(mapped, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main()
{
    vec3 direction = normalize(v_ray);
    // equirect: the image center looks down -z, u grows toward +x, v from the nadir to the zenith
    vec2 uv = vec2(0.5 + atan(direction.x, -direction.z) / (2.0 * PI), 0.5 + asin(direction.y) / PI);
    // the lowest band fades into the fog the terrain ends in, so the horizon has no seam
    float haze = 1.0 - smoothstep(-0.03, 0.16, direction.y);
    vec3 color = mix(texture2D(u_panorama, uv).rgb, u_fog_color, haze);

    gl_FragColor = vec4(tonemap(color * u_exposure), 1.0);
}
