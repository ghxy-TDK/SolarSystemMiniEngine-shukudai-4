#version 330 core

uniform vec4 u_particle_color;
out vec4 frag_color;

void main() {
    vec2  coord = gl_PointCoord * 2.0 - vec2(1.0);
    float d     = dot(coord, coord);
    if (d > 1.0) discard;
    float alpha = u_particle_color.a * (1.0 - d);
    frag_color  = vec4(u_particle_color.rgb, alpha);
}