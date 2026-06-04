#pragma once

#include <string>
#include "../math/vec3.h"
#include "../math/matrix4.h"

// ─────────────────────────────────────────────────────────────────────────────
// Shader — compiles & links one GLSL program from vertex + fragment source
// files.  Must be constructed AFTER glewInit() (i.e. inside Application::init
// or later).  Uniform names are kept strictly in sync with §0.4 of the spec.
// ─────────────────────────────────────────────────────────────────────────────
class Shader {
public:
    // Reads both source files, compiles, links.  Throws std::runtime_error on
    // any GL error so the caller can surface it early during startup.
    Shader(const std::string& vert_path, const std::string& frag_path);

    // Calls glDeleteProgram — safe to call even if compilation failed (guard
    // inside).
    ~Shader();

    // Non-copyable; moveable.
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) noexcept;
    Shader& operator=(Shader&&) noexcept;

    // Bind this program for subsequent draw calls.
    void use() const;

    // ── Uniform setters (§0.4 names enforced by callers) ────────────────────
    void set_int  (const std::string& name, int            v) const;
    void set_float(const std::string& name, float          v) const;
    void set_vec3 (const std::string& name, const Vec3&    v) const;
    void set_mat4 (const std::string& name, const Matrix4& m) const;
    // Accepts a column-major float[9] (matches GL convention).
    void set_mat3 (const std::string& name, const float*   m3) const;

    unsigned int program_id() const { return program_; }

private:
    unsigned int program_ = 0;

    static unsigned int compile_shader(const std::string& path,
                                       unsigned int       type);
    static std::string  read_file(const std::string& path);
    // Checks GL info log and throws on error.
    static void         check_compile(unsigned int shader, const std::string& path);
    static void         check_link   (unsigned int program);
    int                 location     (const std::string& name) const;
};
