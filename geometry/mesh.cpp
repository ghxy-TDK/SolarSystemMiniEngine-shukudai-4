#include "mesh.h"

#include <GL/glew.h>
#include <iostream>
#include <stdexcept>
#include <utility>

// ─────────────────────────────────────────────────────────────────────────────
// Local GL_CHECK (same pattern as shader.cpp — no shared header yet).
// ─────────────────────────────────────────────────────────────────────────────
namespace {

void gl_check_impl(const char* call, const char* file, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[GL_ERROR] 0x" << std::hex << err << std::dec
                  << " at " << call << "  " << file << ":" << line << "\n";
    }
}

} // namespace

#define GL_CHECK(call) do { call; gl_check_impl(#call, __FILE__, __LINE__); } while(0)

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — stores CPU data; no GL calls yet.
// ─────────────────────────────────────────────────────────────────────────────
Mesh::Mesh(std::vector<Vertex> vertices,
    std::vector<unsigned int> indices,
    unsigned int primitive)
    : vertices_(std::move(vertices))
    , indices_(std::move(indices))
    , primitive_(primitive)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Destructor
// ─────────────────────────────────────────────────────────────────────────────
Mesh::~Mesh() {
    if (vao_ != 0) {
        GL_CHECK(glDeleteVertexArrays(1, &vao_));
        GL_CHECK(glDeleteBuffers(1, &vbo_));
        GL_CHECK(glDeleteBuffers(1, &ebo_));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Move semantics
// ─────────────────────────────────────────────────────────────────────────────
Mesh::Mesh(Mesh&& other) noexcept
    : vertices_(std::move(other.vertices_))
    , indices_ (std::move(other.indices_))
    , vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_)
{
    other.vao_ = other.vbo_ = other.ebo_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (vao_ != 0) {
            glDeleteVertexArrays(1, &vao_);
            glDeleteBuffers(1, &vbo_);
            glDeleteBuffers(1, &ebo_);
        }
        vertices_ = std::move(other.vertices_);
        indices_  = std::move(other.indices_);
        vao_ = other.vao_; vbo_ = other.vbo_; ebo_ = other.ebo_;
        other.vao_ = other.vbo_ = other.ebo_ = 0;
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::setup_mesh
//
// VAO layout (matches phong.vert):
//   location 0 — a_position  (vec3  — offset 0)
//   location 1 — a_normal    (vec3  — offset sizeof(Vec3))
//   location 2 — a_tex_coord (vec2  — offset 2*sizeof(Vec3))
// ─────────────────────────────────────────────────────────────────────────────
void Mesh::setup_mesh() {
    if (vao_ != 0) return;  // already uploaded

    GL_CHECK(glGenVertexArrays(1, &vao_));
    GL_CHECK(glGenBuffers(1, &vbo_));
    GL_CHECK(glGenBuffers(1, &ebo_));

    GL_CHECK(glBindVertexArray(vao_));

    // ── VBO ──────────────────────────────────────────────────────────────────
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                          vertices_.data(),
                          GL_STATIC_DRAW));

    // ── EBO ──────────────────────────────────────────────────────────────────
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>(indices_.size() * sizeof(unsigned int)),
                          indices_.data(),
                          GL_STATIC_DRAW));

    // ── Vertex attributes ────────────────────────────────────────────────────
    const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));

    // location 0 : position (vec3)
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                                   reinterpret_cast<void*>(offsetof(Vertex, position))));

    // location 1 : normal (vec3)
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                                   reinterpret_cast<void*>(offsetof(Vertex, normal))));

    // location 2 : tex_coord (vec2 packed as float tex_u, float tex_v)
    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                                   reinterpret_cast<void*>(offsetof(Vertex, tex_u))));

    // Unbind VAO first (keep EBO bound — it's stored in the VAO state).
    GL_CHECK(glBindVertexArray(0));
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh::draw
// ─────────────────────────────────────────────────────────────────────────────
void Mesh::draw() const {
    if (vao_ == 0) {
        std::cerr << "[Mesh] draw() called before setup_mesh()\n";
        return;
    }
    GL_CHECK(glBindVertexArray(vao_));
    GL_CHECK(glDrawElements(primitive_,
        static_cast<GLsizei>(indices_.size()),
        GL_UNSIGNED_INT,
        nullptr));
    GL_CHECK(glBindVertexArray(0));
}
