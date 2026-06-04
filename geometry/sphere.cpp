#include "sphere.h"

#include <cmath>
#include <vector>

// M_PI may not be defined on all MSVC builds.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Sphere::Sphere(float radius, int stacks, int slices)
    : radius_(radius), stacks_(stacks), slices_(slices) {}

std::unique_ptr<Mesh> Sphere::build() const {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    // ── Vertices ─────────────────────────────────────────────────────────────
    // We generate (stacks+1) × (slices+1) vertices so that the seam column is
    // duplicated (tex_u 0 and 1 share the same 3-D position but different UVs).
    for (int stack = 0; stack <= stacks_; ++stack) {
        // phi  ∈ [0, π]   — polar angle from north pole to south pole
        float phi = static_cast<float>(M_PI) * static_cast<float>(stack) /
                    static_cast<float>(stacks_);
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);

        for (int slice = 0; slice <= slices_; ++slice) {
            // theta ∈ [0, 2π]  — azimuthal angle
            float theta = 2.0f * static_cast<float>(M_PI) *
                          static_cast<float>(slice) / static_cast<float>(slices_);
            float sin_theta = std::sin(theta);
            float cos_theta = std::cos(theta);

            // Unit normal equals the outward radial direction on a sphere.
            Vec3 normal{sin_phi * cos_theta, cos_phi, sin_phi * sin_theta};

            Vertex v;
            v.position = Vec3{normal.x * radius_, normal.y * radius_, normal.z * radius_};
            v.normal   = normal;  // already unit length
            v.tex_u    = static_cast<float>(slice) / static_cast<float>(slices_);
            v.tex_v    = static_cast<float>(stack) / static_cast<float>(stacks_);
            verts.push_back(v);
        }
    }

    // ── Indices ───────────────────────────────────────────────────────────────
    // Each quad (stack, slice) → two triangles.
    // Vertex index at (stack, slice) = stack * (slices_+1) + slice.
    for (int stack = 0; stack < stacks_; ++stack) {
        for (int slice = 0; slice < slices_; ++slice) {
            unsigned int top_left     = static_cast<unsigned int>(stack       * (slices_ + 1) + slice);
            unsigned int top_right    = static_cast<unsigned int>(stack       * (slices_ + 1) + slice + 1);
            unsigned int bottom_left  = static_cast<unsigned int>((stack + 1) * (slices_ + 1) + slice);
            unsigned int bottom_right = static_cast<unsigned int>((stack + 1) * (slices_ + 1) + slice + 1);

            // Triangle 1: top-left → bottom-left → bottom-right
            indices.push_back(top_left);
            indices.push_back(bottom_left);
            indices.push_back(bottom_right);

            // Triangle 2: top-left → bottom-right → top-right
            indices.push_back(top_left);
            indices.push_back(bottom_right);
            indices.push_back(top_right);
        }
    }

    return std::make_unique<Mesh>(std::move(verts), std::move(indices));
}
