#version 330 core

// assets/shaders/line.frag
// Outputs a flat uniform color for debug line geometry (orbit rings).

uniform vec3 u_line_color;

out vec4 frag_color;

void main() {
    frag_color = vec4(u_line_color, 1.0);
}
