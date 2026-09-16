#version 120

uniform sampler2D u_source;
uniform vec2 u_direction;

varying vec2 v_uv;

void main()
{
    // discrete Gaussian, sigma 2, radius 4
    vec3 color = texture2D(u_source, v_uv).rgb * 0.2042;

    color += (texture2D(u_source, v_uv + u_direction).rgb + texture2D(u_source, v_uv - u_direction).rgb) * 0.1802;
    color += (texture2D(u_source, v_uv + u_direction * 2.0).rgb +
              texture2D(u_source, v_uv - u_direction * 2.0).rgb) * 0.1238;
    color += (texture2D(u_source, v_uv + u_direction * 3.0).rgb +
              texture2D(u_source, v_uv - u_direction * 3.0).rgb) * 0.0663;
    color += (texture2D(u_source, v_uv + u_direction * 4.0).rgb +
              texture2D(u_source, v_uv - u_direction * 4.0).rgb) * 0.0276;

    gl_FragColor = vec4(color, 1.0);
}
