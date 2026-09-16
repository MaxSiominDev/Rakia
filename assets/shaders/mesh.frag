#version 120

uniform sampler2D u_diffuse_map;
uniform sampler2D u_normal_map;
uniform sampler2D u_metal_rough_map;
uniform sampler2D u_emissive_map;
uniform sampler2D u_panorama;
uniform sampler2DShadow u_shadow_map;
uniform float u_shadow_texel;

uniform vec3 u_kd;
uniform float u_opacity;
uniform vec3 u_emissive;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_alpha_cutoff;
uniform int u_double_sided;
uniform vec3 u_highlight_color;
uniform float u_highlight;

uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;
uniform vec3 u_sky_ambient;
uniform vec3 u_ground_ambient;
uniform vec3 u_fog_color;
uniform float u_fog_density;
uniform vec3 u_camera_position;
uniform float u_sky_yaw;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec4 v_tangent;
varying vec2 v_uv;
varying vec4 v_shadow_coord;

const float PI = 3.14159265;
const float REFLECTION_MAX_LOD = 8.0;
// fade to lit near the box edge, which would otherwise cut shadows along a straight line
const float SHADOW_EDGE_FADE = 0.15;

float sun_visibility()
{
    float edge = min(min(v_shadow_coord.x, 1.0 - v_shadow_coord.x), min(v_shadow_coord.y, 1.0 - v_shadow_coord.y));
    float fade = smoothstep(0.0, SHADOW_EDGE_FADE, edge);
    float lit = 0.0;

    if (fade <= 0.0) {
        return 1.0;
    }

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec4 tap = v_shadow_coord + vec4(float(x), float(y), 0.0, 0.0) * u_shadow_texel;

            lit += shadow2DProj(u_shadow_map, tap).r;
        }
    }

    return mix(1.0, lit / 9.0, fade);
}

vec3 shading_normal()
{
    vec3 normal = normalize(v_normal);
    vec3 tangent = normalize(v_tangent.xyz - normal * dot(normal, v_tangent.xyz));
    vec3 bitangent = cross(normal, tangent) * v_tangent.w;
    vec3 bump = texture2D(u_normal_map, v_uv).xyz * 2.0 - 1.0;

    normal = normalize(tangent * bump.x + bitangent * bump.y + normal * bump.z);
    if (u_double_sided != 0 && !gl_FrontFacing) {
        normal = -normal;
    }

    return normal;
}

// u wraps behind the panorama and its derivative spikes there; u2 has the seam a half turn away,
// so the smaller fwidth of the two corrects the mip bias
vec3 panorama_reflection(vec3 direction, float lod_bias)
{
    float c = cos(u_sky_yaw);
    float s = sin(u_sky_yaw);
    vec3 turned = normalize(vec3(c * direction.x + s * direction.z, direction.y, -s * direction.x + c * direction.z));
    float u = 0.5 + atan(turned.x, -turned.z) / (2.0 * PI);
    float v = 0.5 + asin(turned.y) / PI;
    float u2 = fract(u + 0.5);
    float seam_fix = log2(max(min(fwidth(u), fwidth(u2)), 1e-5) / max(fwidth(u), 1e-5));

    return texture2D(u_panorama, vec2(u, v), seam_fix + lod_bias).rgb;
}

void main()
{
    vec4 diffuse = texture2D(u_diffuse_map, v_uv);
    float alpha = diffuse.a * u_opacity;
    vec3 albedo = mix(diffuse.rgb * u_kd, u_highlight_color, u_highlight);
    vec2 metal_rough = texture2D(u_metal_rough_map, v_uv).bg;
    float metallic = metal_rough.x * u_metallic;
    float roughness = max(metal_rough.y * u_roughness, 0.05);
    vec3 normal = shading_normal();
    vec3 to_camera = normalize(u_camera_position - v_world_position);
    vec3 half_vector = normalize(u_sun_direction + to_camera);
    float n_dot_l = max(dot(normal, u_sun_direction), 0.0);
    float n_dot_h = max(dot(normal, half_vector), 0.0);
    float n_dot_v = max(dot(normal, to_camera), 0.0);
    float v_dot_h = max(dot(to_camera, half_vector), 0.0);
    // roughness 1 is matte, roughness near 0 a tight lobe; the (n + 8) / 8pi factor keeps the lobe's energy level
    float shininess = exp2(11.0 * (1.0 - roughness));
    float lobe = pow(n_dot_h, shininess) * (shininess + 8.0) / (8.0 * PI);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = f0 + (1.0 - f0) * pow(1.0 - v_dot_h, 5.0);
    vec3 fresnel_view = f0 + (1.0 - f0) * pow(1.0 - n_dot_v, 5.0);
    vec3 sun = u_sun_color * n_dot_l * sun_visibility();
    vec3 hemisphere = mix(u_ground_ambient, u_sky_ambient, 0.5 + 0.5 * normal.y);
    vec3 reflection = panorama_reflection(reflect(-to_camera, normal), roughness * REFLECTION_MAX_LOD);
    vec3 color;
    float distance_to_camera;
    float fog;

    if (alpha < u_alpha_cutoff) {
        discard;
    }

    color = albedo * (1.0 - metallic) / PI * (hemisphere + sun) + fresnel * lobe * sun +
            fresnel_view * metallic * reflection;
    color += texture2D(u_emissive_map, v_uv).rgb * u_emissive;
    distance_to_camera = distance(u_camera_position, v_world_position);
    fog = 1.0 - exp(-distance_to_camera * distance_to_camera * u_fog_density * u_fog_density);
    color = mix(color, u_fog_color, fog);

    gl_FragColor = vec4(color, alpha);
}
