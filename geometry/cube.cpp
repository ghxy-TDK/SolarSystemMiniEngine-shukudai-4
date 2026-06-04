#include "cube.h"

#include <vector>
#include <array>

Cube::Cube(float half_size) : half_size_(half_size) {}

std::unique_ptr<Mesh> Cube::build() const {
    const float h = half_size_;

    // Each face is described by its outward normal and four CCW corner vertices.
    // Vertices are listed in CCW order when the face is viewed from outside.
    //
    // Layout per face:
    //   v0 ─── v1
    //   │       │
    //   v3 ─── v2
    //
    // Triangles: (v0, v1, v2) and (v0, v2, v3)

    struct FaceDef {
        Vec3 normal;
        // Four corners in CCW order (viewed from outside).
        std::array<Vec3, 4> corners;
        // UV coordinates matching the corner order.
        std::array<std::array<float,2>, 4> uvs;
    };

    const FaceDef faces[6] = {
        // +Z (front)
        { {0,0,1}, { Vec3{-h,-h, h}, Vec3{ h,-h, h}, Vec3{ h, h, h}, Vec3{-h, h, h} },
          { {{{0,0}},{{1,0}},{{1,1}},{{0,1}}} } },
        // -Z (back)
        { {0,0,-1}, { Vec3{ h,-h,-h}, Vec3{-h,-h,-h}, Vec3{-h, h,-h}, Vec3{ h, h,-h} },
          { {{{0,0}},{{1,0}},{{1,1}},{{0,1}}} } },
        // +X (right)
        { {1,0,0}, { Vec3{ h,-h, h}, Vec3{ h,-h,-h}, Vec3{ h, h,-h}, Vec3{ h, h, h} },
          { {{{0,0}},{{1,0}},{{1,1}},{{0,1}}} } },
        // -X (left)
        { {-1,0,0}, { Vec3{-h,-h,-h}, Vec3{-h,-h, h}, Vec3{-h, h, h}, Vec3{-h, h,-h} },
          { {{{0,0}},{{1,0}},{{1,1}},{{0,1}}} } },
        // +Y (top)
        { {0,1,0}, { Vec3{-h, h, h}, Vec3{ h, h, h}, Vec3{ h, h,-h}, Vec3{-h, h,-h} },
          { {{{0,0}},{{1,0}},{{1,1}},{{0,1}}} } },
        // -Y (bottom)
        { {0,-1,0}, { Vec3{-h,-h,-h}, Vec3{ h,-h,-h}, Vec3{ h,-h, h}, Vec3{-h,-h, h} },
          { {{{0,0}},{{1,0}},{{1,1}},{{0,1}}} } },
    };

    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;
    verts.reserve(24);   // 6 faces × 4 vertices
    indices.reserve(36); // 6 faces × 2 triangles × 3 indices

    for (const auto& face : faces) {
        unsigned int base = static_cast<unsigned int>(verts.size());

        for (int i = 0; i < 4; ++i) {
            Vertex v;
            v.position = face.corners[i];
            v.normal   = face.normal;
            v.tex_u    = face.uvs[i][0];
            v.tex_v    = face.uvs[i][1];
            verts.push_back(v);
        }

        // Two triangles per face (CCW winding): (0,1,2) and (0,2,3).
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    return std::make_unique<Mesh>(std::move(verts), std::move(indices));
}
