#version 120

uniform sampler2D u_ripples;
uniform sampler2D u_panorama;
uniform float u_time;
uniform float u_sky_yaw;

uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;
uniform vec3 u_sky_ambient;
uniform vec3 u_fog_color;
uniform float u_fog_density;
uniform vec3 u_camera_position;

varying vec3 v_world_position;

const float PI = 3.14159265;
// the wave image is 3:2, so its tile is too
const vec2 SWELL_TILE = vec2(64.0, 42.7);
const vec2 CHOP_TILE = vec2(17.0, 11.3);
const float RIPPLE_STRENGTH = 0.22;
// water absorbs the warm end, so what is left of the ambient is green and blue
const vec3 DEEP_COLOR = vec3(0.018, 0.05, 0.062);

vec3 ripple_normal()
{
    vec2 ground = vec2(v_world_position.x, -v_world_position.z);
    vec2 drift = vec2(0.013, -0.021) * u_time;
    vec3 swell = texture2D(u_ripples, ground / SWELL_TILE + drift).xyz * 2.0 - 1.0;
    vec3 chop = texture2D(u_ripples, ground / CHOP_TILE - drift * 1.7).xyz * 2.0 - 1.0;
    vec2 slope = (swell.xy + chop.xy) * RIPPLE_STRENGTH;

    // the plane is horizontal, so the tangent frame is x right, -z forward, y up
    return normalize(vec3(slope.x, 1.0, -slope.y));
}

vec3 sky_reflection(vec3 direction)
{
    float c = cos(u_sky_yaw);
    float s = sin(u_sky_yaw);
    // the panorama is turned the same way the sky shader turns it, and its lower half is not water
    vec3 turned = vec3(c * direction.x + s * direction.z, max(direction.y, 0.0), -s * direction.x + c * direction.z);
    vec2 uv = vec2(0.5 + atan(turned.x, -turned.z) / (2.0 * PI), 0.5 + asin(normalize(turned).y) / PI);

    return texture2D(u_panorama, uv).rgb;
}

void main()
{
    vec3 normal = ripple_normal();
    vec3 to_camera = normalize(u_camera_position - v_world_position);
    vec3 half_vector = normalize(u_sun_direction + to_camera);
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(normal, to_camera), 0.0), 5.0);
    float lobe = pow(max(dot(normal, half_vector), 0.0), 900.0);
    vec3 reflection = sky_reflection(reflect(-to_camera, normal));
    vec3 color = mix(DEEP_COLOR * u_sky_ambient, reflection, fresnel);
    float distance_to_camera = distance(u_camera_position, v_world_position);
    float fog = 1.0 - exp(-distance_to_camera * distance_to_camera * u_fog_density * u_fog_density);

    color += u_sun_color * lobe * fresnel;
    color = mix(color, u_fog_color, fog);

    gl_FragColor = vec4(color, 1.0);
}
