#include "shader.h"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace {
    void gl_check_impl(const char* call, const char* file, int line) {
        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
            std::cerr << "[GL_ERROR] 0x" << std::hex << err << std::dec
            << " at " << call << "  " << file << ":" << line << "\n";
    }

    // Extract const float* from Matrix4::m.
    // m.m is std::array<float,16>; .data() returns the underlying pointer.
    inline const float* mat4_ptr(const Matrix4& m) {
        return m.m.data();
    }
} // namespace
#define GL_CHECK(call) do { call; gl_check_impl(#call,__FILE__,__LINE__); } while(0)

std::string Shader::read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Shader::read_file cannot open: " + path);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void Shader::check_compile(unsigned int shader, const std::string& path) {
    int ok = 0;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &ok));
    if (!ok) {
        char log[1024];
        GL_CHECK(glGetShaderInfoLog(shader, sizeof(log), nullptr, log));
        throw std::runtime_error("Shader compile error [" + path + "]:\n" + log);
    }
}

void Shader::check_link(unsigned int prog) {
    int ok = 0;
    GL_CHECK(glGetProgramiv(prog, GL_LINK_STATUS, &ok));
    if (!ok) {
        char log[1024];
        GL_CHECK(glGetProgramInfoLog(prog, sizeof(log), nullptr, log));
        throw std::runtime_error(std::string("Shader link error:\n") + log);
    }
}

unsigned int Shader::compile_shader(const std::string& path, unsigned int type) {
    std::string  src = read_file(path);
    const char* csrc = src.c_str();
    unsigned int s = 0;
    GL_CHECK(s = glCreateShader(type));
    GL_CHECK(glShaderSource(s, 1, &csrc, nullptr));
    GL_CHECK(glCompileShader(s));
    check_compile(s, path);
    return s;
}

Shader::Shader(const std::string& vert_path, const std::string& frag_path) {
    unsigned int vert = compile_shader(vert_path, GL_VERTEX_SHADER);
    unsigned int frag = compile_shader(frag_path, GL_FRAGMENT_SHADER);
    GL_CHECK(program_ = glCreateProgram());
    GL_CHECK(glAttachShader(program_, vert));
    GL_CHECK(glAttachShader(program_, frag));
    GL_CHECK(glLinkProgram(program_));
    check_link(program_);
    GL_CHECK(glDeleteShader(vert));
    GL_CHECK(glDeleteShader(frag));
}

Shader::~Shader() {
    if (program_ != 0) { glDeleteProgram(program_); program_ = 0; }
}

Shader::Shader(Shader&& o) noexcept : program_(o.program_) { o.program_ = 0; }
Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (program_ != 0) glDeleteProgram(program_);
        program_ = o.program_; o.program_ = 0;
    }
    return *this;
}

void Shader::use() const { GL_CHECK(glUseProgram(program_)); }

int Shader::location(const std::string& name) const {
    int loc = glGetUniformLocation(program_, name.c_str());
    if (loc == -1)
        std::cerr << "[Shader] prog=" << program_
        << " uniform not found: " << name << "\n";
    return loc;
}

void Shader::set_int(const std::string& n, int   v) const { GL_CHECK(glUniform1i(location(n), v)); }
void Shader::set_float(const std::string& n, float v) const { GL_CHECK(glUniform1f(location(n), v)); }
void Shader::set_vec3(const std::string& n, const Vec3& v) const {
    GL_CHECK(glUniform3f(location(n), v.x, v.y, v.z));
}
void Shader::set_vec4(const std::string& name, const Vec4& v) const {
    glUniform4f(glGetUniformLocation(program_, name.c_str()),
        v.x, v.y, v.z, v.w);
}
// std::data(m.m) works for both float[16] (C++17) and std::array<float,16>.
void Shader::set_mat4(const std::string& n, const Matrix4& m) const {
    GL_CHECK(glUniformMatrix4fv(location(n), 1, GL_FALSE, mat4_ptr(m)));
}
void Shader::set_mat3(const std::string& n, const float* m3) const {
    GL_CHECK(glUniformMatrix3fv(location(n), 1, GL_FALSE, m3));
}
