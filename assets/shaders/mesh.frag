#version 120

uniform sampler2D u_diffuse;
uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;
uniform vec3 u_ambient;
uniform vec3 u_camera_position;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;

const float shininess = 32.0;
const float specular_strength = 0.25;

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 to_sun = normalize(u_sun_direction);
    vec3 to_camera = normalize(u_camera_position - v_world_position);
    vec3 half_vector = normalize(to_sun + to_camera);
    float diffuse = max(dot(normal, to_sun), 0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(normal, half_vector), 0.0), shininess) : 0.0;
    vec3 albedo = texture2D(u_diffuse, v_uv).rgb;
    vec3 color = albedo * (u_ambient + u_sun_color * diffuse) + u_sun_color * specular * specular_strength;

    // the window is not an sRGB framebuffer, so the linear result goes back to gamma space here
    gl_FragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
