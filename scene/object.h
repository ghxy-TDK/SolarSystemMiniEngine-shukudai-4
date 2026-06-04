#pragma once
#include "transform.h"
#include "../render/material.h"
#include "../geometry/mesh.h"
#include <memory>
#include <string>
#include <vector>

class Shader;

// ---------------------------------------------------------------------------
// Object
//
// A renderable entity that owns one or more Mesh instances (via unique_ptr)
// and references a shared Material.  The Transform stores local/world TRS.
//
// Ownership:
//   - Object owns its Mesh list exclusively (unique_ptr).
//   - Material is shared (shared_ptr); multiple Objects may share one.
//   - Scene owns Object instances (unique_ptr<Object>).
// ---------------------------------------------------------------------------
class Object {
public:
    explicit Object(std::string name = "");

    // --- data ---
    std::string                 name;
    Transform                   transform;
    std::shared_ptr<Material>   material;

    // --- mesh management ---

    // Takes ownership of the Mesh.  Called by SolarSystem / Model in later stages.
    void add_mesh(std::unique_ptr<Mesh> mesh);

    // --- rendering ---

    // Computes u_model from transform.get_world_matrix(),
    // computes u_normal_matrix (inverse-transpose of the upper-left 3x3),
    // applies material uniforms, then calls draw() on every owned Mesh.
    // Does nothing if material is nullptr.
    void render(Shader& shader) const;

    // --- inspection ---
    std::size_t mesh_count() const { return meshes_.size(); }

private:
    std::vector<std::unique_ptr<Mesh>> meshes_;
};
