#version 120

uniform sampler2D u_atlas;
uniform vec3 u_color;

varying vec2 v_uv;

void main()
{
    // the atlas is white under the coverage alpha, so a filtered edge fades out instead of turning black
    vec4 texel = texture2D(u_atlas, v_uv);

    gl_FragColor = vec4(texel.rgb * u_color, texel.a);
}
