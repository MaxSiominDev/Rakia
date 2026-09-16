#version 120

uniform sampler2D u_source;
uniform vec2 u_texel;
uniform float u_threshold;
uniform float u_knee;

varying vec2 v_uv;

void main()
{
    vec3 a = texture2D(u_source, v_uv + u_texel * vec2(-0.5, -0.5)).rgb;
    vec3 b = texture2D(u_source, v_uv + u_texel * vec2(0.5, -0.5)).rgb;
    vec3 c = texture2D(u_source, v_uv + u_texel * vec2(-0.5, 0.5)).rgb;
    vec3 d = texture2D(u_source, v_uv + u_texel * vec2(0.5, 0.5)).rgb;
    vec3 color = (a + b + c + d) * 0.25;
    float brightness = max(max(color.r, color.g), color.b);
    // soft knee, so a moving pixel does not flicker in and out of the bloom
    float contribution = smoothstep(u_threshold, u_threshold + u_knee, brightness);

    gl_FragColor = vec4(color * contribution, 1.0);
}
