#include "object.h"
#include "../render/shader.h"
#include "../render/material.h"

// Transform::get_local_matrix
Matrix4 Transform::get_local_matrix() const {
    Matrix4 t = Matrix4::translate(position);
    Matrix4 r = Matrix4::rotate(rotation_angle_deg, rotation_axis);
    Matrix4 s = Matrix4::scale(scale);
    return t * r * s;
}

// Transform::get_world_matrix  (recursive parent chain)
Matrix4 Transform::get_world_matrix() const {
    Matrix4 local = get_local_matrix();
    if (parent) return parent->get_world_matrix() * local;
    return local;
}

// Object::render
void Object::render(Shader& shader) const {
    if (material) material->apply(shader);
    for (const auto& mesh : meshes_) {
        if (mesh) mesh->draw();
    }
}
