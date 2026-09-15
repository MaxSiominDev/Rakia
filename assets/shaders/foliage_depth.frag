#version 120

uniform sampler2D u_image;
uniform float u_alpha_cutoff;

varying vec2 v_uv;

void main()
{
    if (texture2D(u_image, v_uv).a < u_alpha_cutoff) {
        discard;
    }
}
