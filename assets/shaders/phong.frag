#version 330 core

// ── Inputs from vertex shader ─────────────────────────────────────────────────
in vec3 v_normal;
in vec3 v_world_pos;
in vec2 v_tex_coord;

// ── Output ────────────────────────────────────────────────────────────────────
out vec4 frag_color;

// ── Material struct (§0.4) ────────────────────────────────────────────────────
struct MaterialData {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};
uniform MaterialData u_material;

// ── Lighting model selector (§0.4) ────────────────────────────────────────────
// 0 = Ambient   1 = Lambert   2 = Phong   3 = Blinn-Phong
// This uniform is reserved for phase 6; the switch below is a skeleton.
uniform int  u_lighting_model;

// ── Point light (§0.4) ───────────────────────────────────────────────────────
struct PointLightData {
    vec3  position;
    vec3  color;
    float intensity;
};
uniform PointLightData u_point_light;

// ── Camera world position (§0.4) ──────────────────────────────────────────────
uniform vec3 u_cam_pos;

// ─────────────────────────────────────────────────────────────────────────────
// main
//
// Phase 3 (basic): output material diffuse colour unconditionally.
// Phase 6 will expand the switch to implement full Phong / Blinn-Phong.
// ─────────────────────────────────────────────────────────────────────────────
void main() {
    // All lighting branches delegated to phase 6.
    // For now, every pixel outputs the material's diffuse colour at full
    // opacity, giving a flat-shaded appearance that is easy to verify.

    // ── Placeholder branch structure (phase 6 will fill each case) ───────────
    vec3 result;
    switch (u_lighting_model) {
        case 0:  // Ambient only
            result = u_material.ambient;
            break;
        case 1:  // Lambert diffuse (phase 6)
            result = u_material.diffuse;   // stub — real Lambert in phase 6
            break;
        case 2:  // Phong specular (phase 6)
            result = u_material.diffuse;   // stub
            break;
        case 3:  // Blinn-Phong (phase 6)
            result = u_material.diffuse;   // stub
            break;
        default:
            result = u_material.diffuse;
            break;
    }

    frag_color = vec4(result, 1.0);
}
