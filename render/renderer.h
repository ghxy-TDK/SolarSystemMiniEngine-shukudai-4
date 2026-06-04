#pragma once

#include <vector>

// Forward declarations — Renderer holds non-owning pointers only.
class Object;
class ParticleSystem;
class Camera;
class Shader;

// ─────────────────────────────────────────────────────────────────────────────
// Renderer — collects non-owning raw pointers each frame and issues draw calls.
//
// Ownership contract (§0.2):
//   • Scene owns all Objects/ParticleSystems via unique_ptr.
//   • Renderer holds raw (non-owning) pointers extracted from Scene.
//   • Renderer MUST NOT delete or extend the lifetime of anything it holds.
//   • Call clear_queue() at the start of every frame before re-submitting.
//
// Usage pattern per frame:
//   renderer.clear_queue();
//   for (auto* obj : scene.get_objects())  renderer.submit_opaque(obj);
//   renderer.render_opaque(camera, phong_shader);
// ─────────────────────────────────────────────────────────────────────────────
class Renderer {
public:
    Renderer()  = default;
    ~Renderer() = default;

    // Non-copyable (pointer queues have frame-local lifetime semantics).
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // ── Submission ────────────────────────────────────────────────────────────
    // Submit an opaque Object for this frame's render pass.
    // Pointer lifetime guaranteed by Scene (the owner).
    void submit_opaque(const Object* obj);

    // Submit a transparent particle system (front-to-back sorting handled
    // inside render_transparent in a later phase).
    void submit_transparent(const ParticleSystem* ps);

    // Submit an Object to the debug wireframe pass (phase 5+).
    void submit_debug(const Object* obj);

    // ── Render passes ─────────────────────────────────────────────────────────
    // Sets u_view, u_projection, u_cam_pos then iterates opaque queue.
    // shader must already be compiled; use() is called internally.
    void render_opaque(const Camera& cam, Shader& shader);

    // Placeholder — transparent rendering (alpha blending + sorting) added in
    // a later phase.  Current implementation is intentionally empty.
    void render_transparent(const Camera& cam, Shader& shader);

    // Placeholder — debug wireframe pass added in a later phase.
    void render_debug(const Camera& cam, Shader& line_shader);

    // Clear all three queues.  Call at the start of each frame.
    void clear_queue();

private:
    std::vector<const Object*>         opaque_queue_;
    std::vector<const ParticleSystem*> transparent_queue_;
    std::vector<const Object*>         debug_queue_;
};
