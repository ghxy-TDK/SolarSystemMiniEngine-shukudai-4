#pragma once

#include <memory>
#include "../geometry/mesh.h"

// Builds a flat circular orbit ring approximated by line segments in the XZ plane.
// The resulting Mesh must be drawn with GL_LINES (see draw_mode()).
//
// NOTE: This class only produces the vertex/index data.
//       The caller is responsible for using line.vert/.frag (provided in stage 11)
//       and must call Mesh::draw() with GL_LINES — see draw_mode() below.
class Orbit {
public:
    // radius   — orbit radius in world units
    // segments — number of line segments approximating the circle (minimum 3)
    explicit Orbit(float radius, int segments = 128);

    // Returns a Mesh where indices encode adjacent pairs for GL_LINES.
    // setup_mesh() must be called on it after the GL context is ready.
    std::unique_ptr<Mesh> build() const;

    // Returns the OpenGL draw mode this geometry requires.
    // Caller must pass this value to the appropriate draw call.
    // Value equals GL_LINES (0x0001).
    static constexpr unsigned int draw_mode() { return 0x0001u; /* GL_LINES */ }

private:
    float radius_;
    int   segments_;
};
