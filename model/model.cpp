// model/model.cpp
// Stage 8: Model implementation.
// All source files must be pure ASCII (MSVC + GBK code page 936).

#include "model.h"
#include "obj_loader.h"
#include "../geometry/mesh.h"   // Mesh
#include "../scene/object.h"    // Object::add_mesh

#include <iostream>
#include <utility>

Model::Model(const std::filesystem::path& path)
    : meshes_(ObjLoader::load(path))
{
    if (meshes_.empty()) {
        std::cerr << "[Model] Warning: no meshes loaded from " << path << "\n";
    }
}

void Model::inject_into(Object& obj) {
    for (auto& mesh : meshes_) {
        obj.add_mesh(std::move(mesh));
    }
    // After moving, each unique_ptr is null; clear the vector.
    meshes_.clear();
}

void Model::setup_all_meshes() {
    for (auto& mesh : meshes_) {
        if (mesh) mesh->setup_mesh();
    }
}