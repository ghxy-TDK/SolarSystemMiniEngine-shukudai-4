#version 330 core

layout(location = 0) in vec3 a_position;

uniform mat4  u_view;
uniform mat4  u_projection;
uniform float u_particle_size;

void main() {
    vec4 view_pos = u_view * vec4(a_position, 1.0);
    gl_Position   = u_projection * view_pos;
    gl_PointSize  = u_particle_size;
}