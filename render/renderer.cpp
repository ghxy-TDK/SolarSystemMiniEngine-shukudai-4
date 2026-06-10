#include <GL/glew.h>
#include <iostream>
#include "../particle/particle_system.h"

#include "renderer.h"

#include "shader.h"
#include "../render/camera.h"   // get_view_matrix / get_projection_matrix
#include "../scene/object.h"    // Object::render, Object::transform

// ─────────────────────────────────────────────────────────────────────────────
// Helpers — compute the normal matrix (upper-left 3×3 of the inverse-transpose
// of the model matrix).  We keep this local to the translation unit so it
// stays testable without dragging in GLM.
// ─────────────────────────────────────────────────────────────────────────────
namespace {

// Compute 3×3 normal matrix from a Matrix4.
// Strategy: brute-force 3×3 inverse-transpose.
// Column-major layout: m[col*4 + row].
void compute_normal_matrix(const Matrix4& model, float out[9]) {
    // Extract upper-left 3×3 (column-major from Matrix4).
    // Column 0
    float a = model.m[0], b = model.m[1], c = model.m[2];
    // Column 1
    float d = model.m[4], e = model.m[5], f = model.m[6];
    // Column 2
    float g = model.m[8], h = model.m[9], k = model.m[10];

    float det = a*(e*k - f*h) - d*(b*k - c*h) + g*(b*f - c*e);

    if (det == 0.f) {
        // Degenerate — fall back to identity to avoid NaN in shader.
        out[0]=1; out[1]=0; out[2]=0;
        out[3]=0; out[4]=1; out[5]=0;
        out[6]=0; out[7]=0; out[8]=1;
        return;
    }

    float inv_det = 1.f / det;

    // Cofactor matrix (= adjugate transposed) then multiply by inv_det.
    // The transpose is the normal matrix (inv-transpose).
    out[0] = inv_det * (e*k - f*h);
    out[1] = inv_det * (-(b*k - c*h));  // transposed position
    out[2] = inv_det * (b*f - c*e);
    out[3] = inv_det * (-(d*k - f*g));
    out[4] = inv_det * (a*k - c*g);
    out[5] = inv_det * (-(a*f - c*d));
    out[6] = inv_det * (d*h - e*g);
    out[7] = inv_det * (-(a*h - b*g));
    out[8] = inv_det * (a*e - b*d);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Submission
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::submit_opaque(const Object* obj) {
    if (obj) opaque_queue_.push_back(obj);
}

void Renderer::submit_transparent(const ParticleSystem* ps) {
    if (ps) transparent_queue_.push_back(ps);
}

void Renderer::submit_debug(const Object* obj) {
    if (obj) debug_queue_.push_back(obj);
}

// ─────────────────────────────────────────────────────────────────────────────
// Renderer::render_opaque
//
// §0.4 uniforms set here:
//   u_view        — camera view matrix
//   u_projection  — camera projection matrix
//   u_cam_pos     — camera world position
//
// Per-object uniforms set inside Object::render():
//   u_model           — world matrix from Transform hierarchy
//   u_normal_matrix   — 3×3 inverse-transpose of model matrix
//   u_material.*      — via Material::apply()
//   u_lighting_model  — lighting model enum (phase 6)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::render_opaque(const Camera& cam, Shader& shader) {
    shader.use();

    // ── Frame-level uniforms (§0.4) ──────────────────────────────────────────
    shader.set_mat4("u_view",       cam.get_view_matrix());
    shader.set_mat4("u_projection", cam.get_projection_matrix());
    shader.set_vec3("u_cam_pos",    cam.position());

    // ── Per-object draw ──────────────────────────────────────────────────────
    for (const Object* obj : opaque_queue_) {
        // Model matrix (includes full parent→child hierarchy via Transform).
        Matrix4 model = obj->transform.get_world_matrix();
        shader.set_mat4("u_model", model);

        // Normal matrix (3×3 inverse-transpose of model matrix).
        float nm[9];
        compute_normal_matrix(model, nm);
        shader.set_mat3("u_normal_matrix", nm);

        // Material uniforms + draw all meshes.
        obj->render(shader);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Renderer::render_debug  — phase 5 (wireframe / normals)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::render_debug(const Camera& cam, Shader& line_shader) {
    if (debug_queue_.empty()) return;

    line_shader.use();
    line_shader.set_mat4("u_view", cam.get_view_matrix());
    line_shader.set_mat4("u_projection", cam.get_projection_matrix());
    line_shader.set_vec3("u_line_color", Vec3{ 0.4f, 0.4f, 0.4f });

    for (const Object* obj : debug_queue_) {
        Matrix4 model = obj->transform.get_world_matrix();
        line_shader.set_mat4("u_model", model);
        obj->draw_meshes_only();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Renderer::render_transparent  — phase 7 (alpha sort + blend)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::render_transparent(const Camera& cam, Shader& shader) {
    if (transparent_queue_.empty()) return;

    shader.use();
    shader.set_mat4("u_view", cam.get_view_matrix());
    shader.set_mat4("u_projection", cam.get_projection_matrix());

    for (const ParticleSystem* ps : transparent_queue_) {
        ps->draw(cam, shader);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Renderer::clear_queue
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::clear_queue() {
    opaque_queue_.clear();
    transparent_queue_.clear();
    debug_queue_.clear();
}
