#version 120

uniform sampler2D u_sheet;
// 1 to shade the sprite with the scene's light (smoke, fire), 0 for one that carries its own color (flash)
uniform int u_lit;

uniform vec3 u_sun_color;
uniform vec3 u_sky_ambient;
uniform vec3 u_ground_ambient;
uniform vec3 u_fog_color;
uniform float u_fog_density;
uniform vec3 u_camera_position;
uniform float u_exposure;

varying vec2 v_uv;
varying vec4 v_color;
varying vec3 v_world_position;

const float PI = 3.14159265;
// flat fraction of the sun added regardless of facing, since a sprite has no surface normal to shade against
const float WRAP = 0.55;

// fitted ACES curve by Krzysztof Narkowicz, the same as in mesh.frag so smoke matches the ground
vec3 tonemap(vec3 color)
{
    vec3 mapped = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);

    return pow(clamp(mapped, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main()
{
    vec4 sprite = texture2D(u_sheet, v_uv);
    vec3 color = sprite.rgb * v_color.rgb;
    float distance_to_camera = distance(u_camera_position, v_world_position);
    float fog = 1.0 - exp(-distance_to_camera * distance_to_camera * u_fog_density * u_fog_density);

    if (u_lit != 0) {
        color = color / PI * (mix(u_ground_ambient, u_sky_ambient, 0.75) + u_sun_color * WRAP);
        color = mix(color, u_fog_color, fog);
    } else {
        // additive blending has no destination to mix fog into, so the color is scaled down by fog instead
        color *= 1.0 - fog;
    }

    gl_FragColor = vec4(tonemap(color * u_exposure), sprite.a * v_color.a);
}
