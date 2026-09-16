#version 120

uniform sampler2D u_scene;
uniform sampler2D u_bloom_half;
uniform sampler2D u_bloom_quarter;
uniform float u_exposure;
uniform float u_bloom_half_strength;
uniform float u_bloom_quarter_strength;

varying vec2 v_uv;

// fitted ACES curve by Krzysztof Narkowicz
vec3 tonemap(vec3 color)
{
    vec3 mapped = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);

    return pow(clamp(mapped, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main()
{
    vec3 scene = texture2D(u_scene, v_uv).rgb;
    vec3 bloom = texture2D(u_bloom_half, v_uv).rgb * u_bloom_half_strength +
                 texture2D(u_bloom_quarter, v_uv).rgb * u_bloom_quarter_strength;

    gl_FragColor = vec4(tonemap((scene + bloom) * u_exposure), 1.0);
}
