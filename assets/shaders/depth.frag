#version 120

uniform sampler2D u_diffuse_map;
uniform float u_opacity;
uniform float u_alpha_cutoff;

varying vec2 v_uv;

void main()
{
    if (texture2D(u_diffuse_map, v_uv).a * u_opacity < u_alpha_cutoff) {
        discard;
    }
}
