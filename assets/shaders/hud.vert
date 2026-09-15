#version 120

// screen pixels to clip space, with the origin in the top left corner and y growing down
uniform mat4 u_projection;

attribute vec2 a_position;
attribute vec2 a_uv;

varying vec2 v_uv;

void main()
{
    v_uv = a_uv;
    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
}
