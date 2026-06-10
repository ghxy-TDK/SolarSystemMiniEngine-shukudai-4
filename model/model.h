#pragma once
// model/model.h
// Stage 8: Model loads an OBJ file and injects meshes into an Object.
// All source files must be pure ASCII (MSVC + GBK code page 936).

#include <filesystem>
#include <memory>
#include <vector>

class Mesh;
class Object;

class Model {
public:
    // Loads geometry from <path> via ObjLoader.
    // setup_mesh() is NOT called on any Mesh here; call inject_into() only
    // after glewInit() has been called (or ensure the caller does so).
    explicit Model(const std::filesystem::path& path);

    // Moves all Mesh ownership into <obj> via Object::add_mesh().
    // After this call meshes_ is empty; the Model object may be destroyed.
    void inject_into(Object& obj);

    // Returns the number of meshes currently owned (0 after inject_into).
    std::size_t mesh_count() const { return meshes_.size(); }

    void setup_all_meshes();  // 在 glewInit 后、inject_into 前调用

private:
    std::vector<std::unique_ptr<Mesh>> meshes_;
};
