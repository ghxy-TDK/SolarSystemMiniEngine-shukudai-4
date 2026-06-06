// model/obj_loader.cpp
// Stage 8: OBJ loader implementation.
// All source files must be pure ASCII (MSVC + GBK code page 936).

#include "obj_loader.h"
#include "../geometry/mesh.h"   // Vertex, Mesh

#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

// ---------------------------------------------------------------------------
// Internal types
// ---------------------------------------------------------------------------

// Raw storage accumulated while scanning the file.
struct ObjRawData {
    std::vector<std::array<float, 3>> positions;   // v
    std::vector<std::array<float, 2>> texcoords;   // vt
    std::vector<std::array<float, 3>> normals;     // vn
};

// One index triple as it appears after the slash-separated face token.
// Zero means "not present" (OBJ indices are 1-based, so 0 is safe sentinel).
struct FaceIndex {
    int vi  = 0;   // vertex index (1-based)
    int vti = 0;   // texcoord index (1-based), 0 = absent
    int vni = 0;   // normal index (1-based), 0 = absent
};

// A group collects all face-corner indices belonging to one o/g section.
struct Group {
    std::string name;
    std::vector<std::vector<FaceIndex>> faces;   // each face is 3..4 corners
};

// ---------------------------------------------------------------------------
// Parsing helpers
// ---------------------------------------------------------------------------

// Parse a single "v/vt/vn", "v//vn", "v/vt", or plain "v" token.
static FaceIndex parse_face_token(const std::string& tok) {
    FaceIndex fi;
    // Count slashes to choose branch.
    auto first_slash = tok.find('/');
    if (first_slash == std::string::npos) {
        // Format: v
        fi.vi = std::stoi(tok);
        return fi;
    }
    fi.vi = std::stoi(tok.substr(0, first_slash));

    auto second_slash = tok.find('/', first_slash + 1);
    if (second_slash == std::string::npos) {
        // Format: v/vt
        std::string vt_str = tok.substr(first_slash + 1);
        if (!vt_str.empty()) fi.vti = std::stoi(vt_str);
        return fi;
    }

    // Two slashes present.
    std::string vt_str = tok.substr(first_slash + 1, second_slash - first_slash - 1);
    std::string vn_str = tok.substr(second_slash + 1);
    if (!vt_str.empty()) fi.vti = std::stoi(vt_str);
    if (!vn_str.empty()) fi.vni = std::stoi(vn_str);
    return fi;
}

// OBJ supports negative indices (relative from end of current list).
// Resolve to a non-negative 0-based index. Returns -1 on out-of-range.
static int resolve_index(int raw, int list_size) {
    if (raw == 0) return -1;               // absent
    int idx = (raw > 0) ? (raw - 1) : (list_size + raw);
    if (idx < 0 || idx >= list_size) return -1;
    return idx;
}

// ---------------------------------------------------------------------------
// Mesh builder for one group
// ---------------------------------------------------------------------------

static std::unique_ptr<Mesh> build_mesh(
    const Group& group,
    const ObjRawData& raw)
{
    // Key = (vi, vti, vni) as resolved 0-based indices (-1 when absent).
    using Key = std::tuple<int, int, int>;
    std::map<Key, unsigned int> index_map;

    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;

    // Flat normal fallback (used when vni is absent for a given corner).
    // We compute a per-face geometric normal and assign it uniformly.
    auto resolve_vertex = [&](const FaceIndex& fi,
                              const Vec3& face_normal) -> unsigned int
    {
        int vi  = resolve_index(fi.vi,  static_cast<int>(raw.positions.size()));
        int vti = resolve_index(fi.vti, static_cast<int>(raw.texcoords.size()));
        int vni = resolve_index(fi.vni, static_cast<int>(raw.normals.size()));

        // For deduplication purposes, use the actual resolved indices.
        Key key{ vi, vti, vni };
        auto it = index_map.find(key);
        if (it != index_map.end()) return it->second;

        Vertex v{};

        // Position
        if (vi >= 0) {
            v.position = Vec3{ raw.positions[vi][0],
                               raw.positions[vi][1],
                               raw.positions[vi][2] };
        }

        // Normal: prefer explicit vn, else use face geometric normal.
        if (vni >= 0) {
            v.normal = Vec3{ raw.normals[vni][0],
                             raw.normals[vni][1],
                             raw.normals[vni][2] };
        } else {
            v.normal = face_normal;
        }

        // Texcoord
        if (vti >= 0) {
            v.tex_u = raw.texcoords[vti][0];
            v.tex_v = raw.texcoords[vti][1];
        }

        unsigned int new_idx = static_cast<unsigned int>(verts.size());
        verts.push_back(v);
        index_map[key] = new_idx;
        return new_idx;
    };

    for (const auto& face : group.faces) {
        // Need at least 3 corners to form a triangle.
        if (face.size() < 3) continue;

        // Compute geometric normal from first three corners (for fallback).
        Vec3 geom_normal{ 0.f, 1.f, 0.f };
        {
            int vi0 = resolve_index(face[0].vi, static_cast<int>(raw.positions.size()));
            int vi1 = resolve_index(face[1].vi, static_cast<int>(raw.positions.size()));
            int vi2 = resolve_index(face[2].vi, static_cast<int>(raw.positions.size()));
            if (vi0 >= 0 && vi1 >= 0 && vi2 >= 0) {
                Vec3 p0{ raw.positions[vi0][0], raw.positions[vi0][1], raw.positions[vi0][2] };
                Vec3 p1{ raw.positions[vi1][0], raw.positions[vi1][1], raw.positions[vi1][2] };
                Vec3 p2{ raw.positions[vi2][0], raw.positions[vi2][1], raw.positions[vi2][2] };
                Vec3 edge1 = p1 + (p0 * -1.f);  // p1 - p0
                Vec3 edge2 = p2 + (p0 * -1.f);  // p2 - p0
                Vec3 cross = edge1.cross(edge2);
                float len = cross.length();
                if (len > 1e-6f) geom_normal = cross * (1.f / len);
            }
        }

        // Fan triangulation: (0,1,2), (0,2,3), (0,3,4), ...
        unsigned int i0 = resolve_vertex(face[0], geom_normal);
        for (std::size_t k = 1; k + 1 < face.size(); ++k) {
            unsigned int ik  = resolve_vertex(face[k],     geom_normal);
            unsigned int ik1 = resolve_vertex(face[k + 1], geom_normal);
            indices.push_back(i0);
            indices.push_back(ik);
            indices.push_back(ik1);
        }
    }

    if (verts.empty()) return nullptr;

    // Construct Mesh; setup_mesh() is deliberately NOT called here.
    // The caller must call it after glewInit().
    return std::make_unique<Mesh>(std::move(verts), std::move(indices));
}

// ---------------------------------------------------------------------------
// ObjLoader::load
// ---------------------------------------------------------------------------

std::vector<std::unique_ptr<Mesh>> ObjLoader::load(
    const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ObjLoader] Cannot open: " << path << "\n";
        return {};
    }

    ObjRawData raw;

    // Start with a default group so faces before the first 'o'/'g' are captured.
    std::vector<Group> groups;
    groups.push_back(Group{ "default", {} });

    std::string line;
    while (std::getline(file, line)) {
        // Strip carriage return (Windows line endings).
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Skip empty lines and comments.
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            raw.positions.push_back({ x, y, z });
        }
        else if (token == "vt") {
            float u, v;
            ss >> u >> v;
            raw.texcoords.push_back({ u, v });
        }
        else if (token == "vn") {
            float nx, ny, nz;
            ss >> nx >> ny >> nz;
            raw.normals.push_back({ nx, ny, nz });
        }
        else if (token == "o" || token == "g") {
            // Start a new group. Keep the old one only if it has faces.
            std::string name;
            ss >> name;
            if (name.empty()) name = "unnamed";

            // Push a new group unconditionally; empty groups are pruned later.
            groups.push_back(Group{ name, {} });
        }
        else if (token == "f") {
            std::vector<FaceIndex> face;
            std::string face_tok;
            while (ss >> face_tok) {
                // Basic guard against corrupt tokens.
                if (face_tok.empty()) continue;
                try {
                    face.push_back(parse_face_token(face_tok));
                } catch (...) {
                    std::cerr << "[ObjLoader] Bad face token: " << face_tok << "\n";
                }
            }
            if (face.size() >= 3) {
                groups.back().faces.push_back(std::move(face));
            }
        }
        // All other directives (mtllib, usemtl, s, etc.) are ignored.
    }

    // Build one Mesh per non-empty group.
    std::vector<std::unique_ptr<Mesh>> result;
    for (const auto& grp : groups) {
        if (grp.faces.empty()) continue;
        auto mesh = build_mesh(grp, raw);
        if (mesh) result.push_back(std::move(mesh));
    }

    if (result.empty()) {
        std::cerr << "[ObjLoader] No geometry found in: " << path << "\n";
    }

    return result;
}
