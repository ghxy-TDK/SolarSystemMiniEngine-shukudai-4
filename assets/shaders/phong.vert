#version 330 core

// ── Vertex attributes (§0.4 / mesh.cpp layout) ───────────────────────────────
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_tex_coord;

// ── Uniforms (§0.4) ──────────────────────────────────────────────────────────
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normal_matrix;   // inverse-transpose of upper-left 3×3 of model

// ── Outputs to fragment shader ────────────────────────────────────────────────
out vec3 v_normal;      // surface normal in world space
out vec3 v_world_pos;   // fragment position in world space
out vec2 v_tex_coord;   // passed through for future texture sampling

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos    = world_pos.xyz;

    // Transform normal to world space using the normal matrix.
    // The normal matrix is the inverse-transpose of the model's 3×3 upper-left,
    // which correctly handles non-uniform scaling.
    v_normal   = normalize(u_normal_matrix * a_normal);

    v_tex_coord = a_tex_coord;

    gl_Position = u_projection * u_view * world_pos;
}
