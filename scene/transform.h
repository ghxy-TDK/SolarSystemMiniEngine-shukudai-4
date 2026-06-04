#pragma once
#include "../math/vec3.h"
#include "../math/matrix4.h"

// ---------------------------------------------------------------------------
// Transform
//
// Stores local TRS (translate / rotate / scale) for a scene node.
// Supports a non-owning parent pointer for a simple parent-child hierarchy.
// World matrix = parent->get_world_matrix() * get_local_matrix().
// ---------------------------------------------------------------------------
class Transform {
public:
    Vec3  position         = {0.f, 0.f, 0.f};
    Vec3  scale            = {1.f, 1.f, 1.f};
    Vec3  rotation_axis    = {0.f, 1.f, 0.f};
    float rotation_angle_deg = 0.f;

    // Non-owning pointer to the parent transform.
    // Lifetime of *parent must exceed lifetime of this Transform.
    Transform* parent = nullptr;

    // Returns the local TRS matrix: T * R * S
    Matrix4 get_local_matrix() const;

    // Returns the world matrix, recursively composing parent chain.
    // Root node (parent == nullptr) returns get_local_matrix().
    Matrix4 get_world_matrix() const;
};
