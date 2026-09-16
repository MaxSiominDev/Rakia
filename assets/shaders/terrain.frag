#version 120

// 0 dune sand, 1 stony ground, 2 rock cliff, 3 grass, 4 dry grass; size matches TERRAIN_TEXTURE_SETS
uniform sampler2D u_diffuse[5];
uniform sampler2D u_normal[5];
uniform sampler2D u_rough[5];
uniform sampler2DShadow u_shadow_map;
uniform float u_shadow_texel;

uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;
uniform vec3 u_sky_ambient;
uniform vec3 u_ground_ambient;
uniform vec3 u_fog_color;
uniform float u_fog_density;
uniform vec3 u_camera_position;

varying vec3 v_world_position;
varying vec3 v_normal;
varying vec4 v_tangent;
varying vec2 v_blend;
varying vec4 v_shadow_coord;

const float PI = 3.14159265;
// fade to lit near the box edge, which would otherwise cut shadows along a straight line
const float SHADOW_EDGE_FADE = 0.15;
// metres one tile of each set covers
const vec4 TILE = vec4(16.0, 22.0, 13.0, 8.0);
const float DRY_TILE = 9.0;
// mirrors TERRAIN_SEA_LEVEL in terrain.h
const float SEA_LEVEL = -30.0;
const float WET_SAND_REACH = 5.0;
const float FOAM_REACH = 0.6;
const vec3 WET_SAND_TINT = vec3(0.55, 0.50, 0.40);
const vec3 FOAM_COLOR = vec3(0.88, 0.90, 0.86);
// the grass set alone reads as flat lime
const vec3 GRASS_TINT = vec3(0.85, 0.90, 0.60);
const float DRY_PATCH_SCALE = 160.0;

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

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float value_noise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash(cell);
    float b = hash(cell + vec2(1.0, 0.0));
    float c = hash(cell + vec2(0.0, 1.0));
    float d = hash(cell + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// warped by a coarser field so the cell grid does not show from altitude
float dry_field(vec2 p)
{
    vec2 warp = vec2(value_noise(p * 0.4 + vec2(11.3, 5.7)), value_noise(p * 0.4 + vec2(-3.1, 8.9)));

    return value_noise(p + (warp - 0.5) * 3.0);
}

void main()
{
    // v grows toward -z, which keeps the tangent frame right-handed under the +y normal
    vec2 ground = vec2(v_world_position.x, -v_world_position.z);
    vec2 sand_uv = ground / TILE.x;
    vec2 stone_uv = ground / TILE.y;
    vec2 cliff_uv = ground / TILE.z;
    vec2 grass_uv = ground / TILE.w;
    vec2 dry_uv = ground / DRY_TILE;
    vec3 normal = normalize(v_normal);
    float cliff = smoothstep(0.12, 0.40, 1.0 - normal.y);
    float stone = v_blend.y;
    float grass = v_blend.x * (1.0 - 0.5 * stone);
    float dry_patch = smoothstep(0.25, 0.75, dry_field(ground / DRY_PATCH_SCALE));
    vec3 grass_albedo = mix(texture2D(u_diffuse[3], grass_uv).rgb, texture2D(u_diffuse[4], dry_uv).rgb, dry_patch) *
                        GRASS_TINT;
    vec3 grass_bump = mix(texture2D(u_normal[3], grass_uv).xyz, texture2D(u_normal[4], dry_uv).xyz, dry_patch);
    float grass_rough = mix(texture2D(u_rough[3], grass_uv).g, texture2D(u_rough[4], dry_uv).g, dry_patch);
    float above_sea = v_world_position.y - SEA_LEVEL;
    vec3 albedo = texture2D(u_diffuse[0], sand_uv).rgb;
    vec3 bump = texture2D(u_normal[0], sand_uv).xyz;
    float roughness = texture2D(u_rough[0], sand_uv).g;

    albedo = mix(albedo, texture2D(u_diffuse[1], stone_uv).rgb, stone);
    bump = mix(bump, texture2D(u_normal[1], stone_uv).xyz, stone);
    roughness = mix(roughness, texture2D(u_rough[1], stone_uv).g, stone);

    albedo = mix(albedo, grass_albedo, grass);
    bump = mix(bump, grass_bump, grass);
    roughness = mix(roughness, grass_rough, grass);

    albedo = mix(albedo, texture2D(u_diffuse[2], cliff_uv).rgb, cliff);
    bump = mix(bump, texture2D(u_normal[2], cliff_uv).xyz, cliff);
    roughness = mix(roughness, texture2D(u_rough[2], cliff_uv).g, cliff);

    // wet sand and foam at sea level hide the mesh steps along the waterline; steep slopes stay bare
    albedo = mix(albedo, albedo * WET_SAND_TINT, (1.0 - smoothstep(0.0, WET_SAND_REACH, above_sea)) * (1.0 - cliff));
    albedo = mix(albedo, FOAM_COLOR, (1.0 - smoothstep(0.0, FOAM_REACH, abs(above_sea))) * (1.0 - cliff) * 0.5);

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

    gl_FragColor = vec4(mix(color, u_fog_color, fog), 1.0);
}
