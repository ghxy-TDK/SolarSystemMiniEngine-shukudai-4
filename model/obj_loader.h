#pragma once
// model/obj_loader.h
// Stage 8: OBJ file loader. Pure CPU parsing, no GL calls.
// All source files must be pure ASCII (MSVC + GBK code page 936).

#include <filesystem>
#include <memory>
#include <vector>

// Forward declaration to avoid pulling in geometry/mesh.h indirectly.
// Callers must include geometry/mesh.h themselves before using Mesh members.
class Mesh;

class ObjLoader {
public:
    // Parses the .obj file at <path> and returns one Mesh per face group (o/g).
    // Supports v / vt / vn / f directives.
    // Faces may be triangles or quads; quads are fan-triangulated.
    // Vertices are deduplicated per group using a (vi, vti, vni) index tuple.
    // Returns an empty vector on any parse error (error is printed to stderr).
    // NOTE: Mesh::setup_mesh() is NOT called here; the caller must invoke it
    //       after a valid GL context is available (i.e. after glewInit()).
    static std::vector<std::unique_ptr<Mesh>> load(
        const std::filesystem::path& path);
};
