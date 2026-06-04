#pragma once
// scene/object.h
// Phase 3 minimal stub -- expanded in Phase 4 (scene/object.cpp).
// Provides enough interface for Renderer to call render() and access transform.
#include <memory>
#include <vector>
#include "../math/matrix4.h"
#include "../render/material.h"
#include "../geometry/mesh.h"

// Forward declaration from scene/transform.h (defined in Phase 2).
// Provide a minimal version here so Phase 3 compiles standalone.
#ifndef TRANSFORM_DEFINED
#define TRANSFORM_DEFINED
#include "../math/vec3.h"
struct Transform {
    Vec3  position = {0,0,0};
    Vec3  scale    = {1,1,1};
    Vec3  rotation_axis = {0,1,0};
    float rotation_angle_deg = 0.f;
    Transform* parent = nullptr;

    Matrix4 get_local_matrix() const;
    Matrix4 get_world_matrix() const;
};
#endif

class Object {
public:
    Transform              transform;
    std::shared_ptr<Material> material;

    void add_mesh(std::unique_ptr<Mesh> mesh) {
        meshes_.push_back(std::move(mesh));
    }

    // Sets u_material.* uniforms then calls Mesh::draw() for every mesh.
    void render(Shader& shader) const;

private:
    std::vector<std::unique_ptr<Mesh>> meshes_;
};
