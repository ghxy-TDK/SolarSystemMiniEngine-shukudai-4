#pragma once

#include <vector>
#include "../math/vec3.h"

// ─────────────────────────────────────────────────────────────────────────────
// Vertex — interleaved layout sent to the GPU.
//   location 0 : position  (vec3)
//   location 1 : normal    (vec3)
//   location 2 : tex_coord (vec2  — stored as two floats)
// ─────────────────────────────────────────────────────────────────────────────
struct Vertex {
    Vec3  position;
    Vec3  normal;
    float tex_u = 0.f;
    float tex_v = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Mesh — owns a VAO/VBO/EBO triple.  Must call setup_mesh() once a valid GL
// context exists (i.e. after Application::init()).  Owned exclusively by an
// Object via unique_ptr<Mesh>.
// ─────────────────────────────────────────────────────────────────────────────
class Mesh {
public:
    Mesh(std::vector<Vertex>       vertices,
         std::vector<unsigned int> indices);
    ~Mesh();  // glDeleteVertexArrays + glDeleteBuffers

    // Non-copyable (GL handles are not ref-counted here).
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;

    // Upload vertex/index data to the GPU.  Must be called once, after
    // glewInit().  Calling a second time is a no-op (guard via vao_ != 0).
    void setup_mesh();

    // glBindVertexArray(vao_) + glDrawElements.  Assumes setup_mesh() was
    // called and the correct Shader is already bound.
    void draw() const;

    // Read-only access (useful for ray-tracing queries).
    const std::vector<Vertex>&       vertices() const { return vertices_; }
    const std::vector<unsigned int>& indices()  const { return indices_;  }

private:
    std::vector<Vertex>       vertices_;
    std::vector<unsigned int> indices_;

    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
};
