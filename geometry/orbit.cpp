#include <GL/glew.h>

#include "orbit.h"

#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Orbit::Orbit(float radius, int segments)
    : radius_(radius), segments_(segments) {}

std::unique_ptr<Mesh> Orbit::build() const {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    verts.reserve(static_cast<size_t>(segments_));
    // GL_LINES draws one segment per pair of indices → 2 indices per segment,
    // and the ring closes by connecting the last vertex back to vertex 0.
    indices.reserve(static_cast<size_t>(segments_) * 2);

    for (int i = 0; i < segments_; ++i) {
        float theta = 2.0f * static_cast<float>(M_PI) *
                      static_cast<float>(i) / static_cast<float>(segments_);

        Vertex v;
        // Orbit lies in the XZ plane (Y = 0), consistent with a top-down solar system view.
        v.position = Vec3{ radius_ * std::cos(theta), 0.0f, radius_ * std::sin(theta) };
        v.normal   = Vec3{ 0.0f, 1.0f, 0.0f }; // upward normal (unused by line shader)
        v.tex_u    = static_cast<float>(i) / static_cast<float>(segments_);
        v.tex_v    = 0.0f;
        verts.push_back(v);
    }

    // Build index pairs for GL_LINES: each segment connects vertex i to vertex (i+1)%N.
    for (int i = 0; i < segments_; ++i) {
        indices.push_back(static_cast<unsigned int>(i));
        indices.push_back(static_cast<unsigned int>((i + 1) % segments_));
    }

    return std::make_unique<Mesh>(std::move(verts), std::move(indices), GL_LINES);
}
