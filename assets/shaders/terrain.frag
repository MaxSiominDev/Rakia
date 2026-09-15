#version 120

// 0 dune sand, 1 stony ground, 2 rock cliff, 3 northern grass
uniform sampler2D u_diffuse[4];
uniform sampler2D u_normal[4];
uniform sampler2D u_rough[4];
uniform sampler2DShadow u_shadow_map;
uniform float u_shadow_texel;

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
varying vec4 v_tangent;
varying vec2 v_blend;
varying vec4 v_shadow_coord;

const float PI = 3.14159265;
// metres one tile of each set covers
const vec4 TILE = vec4(16.0, 22.0, 13.0, 8.0);

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

// fitted ACES curve by Krzysztof Narkowicz, the same as in mesh.frag so terrain and objects match
vec3 tonemap(vec3 color)
{
    vec3 mapped = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);

    return pow(clamp(mapped, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main()
{
    // v grows toward -z, which keeps the tangent frame right-handed under the +y normal
    vec2 ground = vec2(v_world_position.x, -v_world_position.z);
    vec2 sand_uv = ground / TILE.x;
    vec2 stone_uv = ground / TILE.y;
    vec2 cliff_uv = ground / TILE.z;
    vec2 grass_uv = ground / TILE.w;
    vec3 normal = normalize(v_normal);
    float cliff = smoothstep(0.12, 0.40, 1.0 - normal.y);
    float stone = v_blend.y;
    float grass = v_blend.x * (1.0 - 0.5 * stone);
    vec3 albedo = texture2D(u_diffuse[0], sand_uv).rgb;
    vec3 bump = texture2D(u_normal[0], sand_uv).xyz;
    float roughness = texture2D(u_rough[0], sand_uv).g;

    albedo = mix(albedo, texture2D(u_diffuse[1], stone_uv).rgb, stone);
    bump = mix(bump, texture2D(u_normal[1], stone_uv).xyz, stone);
    roughness = mix(roughness, texture2D(u_rough[1], stone_uv).g, stone);

    albedo = mix(albedo, texture2D(u_diffuse[3], grass_uv).rgb, grass);
    bump = mix(bump, texture2D(u_normal[3], grass_uv).xyz, grass);
    roughness = mix(roughness, texture2D(u_rough[3], grass_uv).g, grass);

    albedo = mix(albedo, texture2D(u_diffuse[2], cliff_uv).rgb, cliff);
    bump = mix(bump, texture2D(u_normal[2], cliff_uv).xyz, cliff);
    roughness = mix(roughness, texture2D(u_rough[2], cliff_uv).g, cliff);

    vec3 tangent = normalize(v_tangent.xyz - normal * dot(normal, v_tangent.xyz));
    vec3 bitangent = cross(normal, tangent) * v_tangent.w;
    vec3 detail = bump * 2.0 - 1.0;
    vec3 shading_normal = normalize(tangent * detail.x + bitangent * detail.y + normal * detail.z);
    vec3 to_camera = normalize(u_camera_position - v_world_position);
    vec3 half_vector = normalize(u_sun_direction + to_camera);
    float shininess = exp2(11.0 * (1.0 - max(roughness, 0.25)));
    float lobe = pow(max(dot(shading_normal, half_vector), 0.0), shininess) * (shininess + 8.0) / (8.0 * PI);
    vec3 fresnel = vec3(0.04) + vec3(0.96) * pow(1.0 - max(dot(to_camera, half_vector), 0.0), 5.0);
    vec3 sun = u_sun_color * max(dot(shading_normal, u_sun_direction), 0.0) * sun_visibility();
    vec3 hemisphere = mix(u_ground_ambient, u_sky_ambient, 0.5 + 0.5 * shading_normal.y);
    vec3 color = albedo / PI * (hemisphere + sun) + fresnel * lobe * sun;
    float distance_to_camera = distance(u_camera_position, v_world_position);
    float fog = 1.0 - exp(-distance_to_camera * distance_to_camera * u_fog_density * u_fog_density);

    gl_FragColor = vec4(tonemap(mix(color, u_fog_color, fog) * u_exposure), 1.0);
}
