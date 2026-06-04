#pragma once

#include <memory>
#include "../geometry/mesh.h"

// Builds a cube Mesh centred at the origin.
// Each of the 6 faces has its own 4 vertices so normals are hard (face-flat).
// Face winding is counter-clockwise when viewed from outside.
class Cube {
public:
    // half_size — half the edge length; the cube spans [-half, +half] on each axis.
    explicit Cube(float half_size = 0.5f);

    // Returns a fully-built Mesh ready for setup_mesh() after GL context is live.
    std::unique_ptr<Mesh> build() const;

private:
    float half_size_;
};
