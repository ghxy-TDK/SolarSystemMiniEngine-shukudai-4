#pragma once

#include <memory>
#include "../geometry/mesh.h"

// Builds a UV sphere Mesh from spherical coordinate parameterization.
// Stacks = horizontal rings (latitude), slices = vertical segments (longitude).
// Normals are outward unit radial vectors; UVs follow standard lat/lon mapping.
class Sphere {
public:
    // radius  — sphere radius in world units
    // stacks  — number of horizontal bands (minimum 2)
    // slices  — number of vertical segments per band (minimum 3)
    Sphere(float radius, int stacks, int slices);

    // Returns a fully-built Mesh ready for setup_mesh() after GL context is live.
    std::unique_ptr<Mesh> build() const;

private:
    float radius_;
    int   stacks_;
    int   slices_;
};
