#include "object.h"
#include "../render/shader.h"

Object::Object(std::string n)
    : name(std::move(n))
    , material(std::make_shared<Material>())
{}

void Object::add_mesh(std::unique_ptr<Mesh> mesh) {
    meshes_.push_back(std::move(mesh));
}

void Object::render(Shader& shader) const {
    if (!material) return;

    Matrix4 world = transform.get_world_matrix();
    shader.set_mat4("u_model", world);

    float a00 = world.m[0], a10 = world.m[1], a20 = world.m[2];
    float a01 = world.m[4], a11 = world.m[5], a21 = world.m[6];
    float a02 = world.m[8], a12 = world.m[9], a22 = world.m[10];

    float det = a00 * (a11 * a22 - a21 * a12)
              - a01 * (a10 * a22 - a20 * a12)
              + a02 * (a10 * a21 - a20 * a11);
    float inv = (det != 0.f) ? 1.f / det : 0.f;

    float nm[9];
    nm[0] = (a11*a22 - a21*a12) * inv;
    nm[1] = (a21*a02 - a01*a22) * inv;
    nm[2] = (a01*a12 - a11*a02) * inv;
    nm[3] = (a20*a12 - a10*a22) * inv;
    nm[4] = (a00*a22 - a20*a02) * inv;
    nm[5] = (a10*a02 - a00*a12) * inv;
    nm[6] = (a10*a21 - a20*a11) * inv;
    nm[7] = (a20*a01 - a00*a21) * inv;
    nm[8] = (a00*a11 - a10*a01) * inv;

    shader.set_mat3("u_normal_matrix", nm);
    material->apply(shader);
    for (const auto& mesh : meshes_) {
        mesh->draw();
    }
}

void Object::draw_meshes_only() const {
    for (const auto& mesh : meshes_) {
        mesh->draw();
    }
}