#include "transform.h"

Matrix4 Transform::get_local_matrix() const {
    Matrix4 t = Matrix4::translate(position);
    Matrix4 r = Matrix4::rotate(rotation_angle_deg, rotation_axis);
    Matrix4 s = Matrix4::scale(scale);
    // Column-major composition: T * R * S
    return t * r * s;
}

Matrix4 Transform::get_world_matrix() const {
    Matrix4 local = get_local_matrix();
    if (parent == nullptr) {
        return local;
    }
    return parent->get_world_matrix() * local;
}
