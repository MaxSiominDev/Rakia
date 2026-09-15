#version 120

uniform sampler2D u_image;
uniform sampler2DShadow u_shadow_map;
uniform float u_shadow_texel;
uniform float u_alpha_cutoff;

uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;
uniform vec3 u_sky_ambient;
uniform vec3 u_ground_ambient;
uniform vec3 u_fog_color;
uniform float u_fog_density;
uniform vec3 u_camera_position;
uniform float u_exposure;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_shadow_coord;

const float PI = 3.14159265;

float sun_visibility()
{
    float lit = 0.0;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec4 tap = v_shadow_coord + vec4(float(x), float(y), 0.0, 0.0) * u_shadow_texel;

            lit += shadow2DProj(u_shadow_map, tap).r;
        }
    }

    return lit / 9.0;
}

// fitted ACES curve by Krzysztof Narkowicz, the same as in mesh.frag so plants match the ground
vec3 tonemap(vec3 color)
{
    vec3 mapped = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);

    return pow(clamp(mapped, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main()
{
    vec4 cutout = texture2D(u_image, v_uv);
    vec3 normal = normalize(v_normal);
    vec3 sun = u_sun_color * max(dot(normal, u_sun_direction), 0.0) * sun_visibility();
    vec3 hemisphere = mix(u_ground_ambient, u_sky_ambient, 0.5 + 0.5 * normal.y);
    float distance_to_camera = distance(u_camera_position, v_world_position);
    float fog = 1.0 - exp(-distance_to_camera * distance_to_camera * u_fog_density * u_fog_density);
    vec3 color;

    if (cutout.a < u_alpha_cutoff) {
        discard;
    }

    color = cutout.rgb / PI * (hemisphere + sun);
    color = mix(color, u_fog_color, fog);

    gl_FragColor = vec4(tonemap(color * u_exposure), 1.0);
}
