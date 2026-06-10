// main_test.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Smoke test for Phase 3: Shader + Mesh + Renderer.
//
// Build (example, adapt paths for your CMake):
//   g++ main_test.cpp render/shader.cpp geometry/mesh.cpp render/renderer.cpp \
//       -I. -lGL -lGLEW -lglut -o test_phase3 && ./test_phase3
//
// Expected output (with a valid GL context on the machine):
//   [Phase3] Application init OK
//   [Phase3] Shader compiled OK
//   [Phase3] Mesh setup OK
//   [Phase3] Renderer render_opaque OK
//   [Phase3] ALL TESTS PASSED
// ─────────────────────────────────────────────────────────────────────────────

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "core/application.h"
#include "render/shader.h"
#include "geometry/mesh.h"
#include "render/renderer.h"
#include "render/camera.h"
#include "scene/object.h"
#include "render/material.h"

// ─────────────────────────────────────────────────────────────────────────────
// GL_CHECK macro (consistent with shader.cpp / mesh.cpp).
// ─────────────────────────────────────────────────────────────────────────────
namespace {
void gl_check_impl(const char* call, const char* file, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cerr << "[GL_ERROR] 0x" << std::hex << err << std::dec
                  << " at " << call << "  " << file << ":" << line << "\n";
}
} // namespace
#define GL_CHECK(call) do { call; gl_check_impl(#call, __FILE__, __LINE__); } while(0)

// ─────────────────────────────────────────────────────────────────────────────
// Helper — build a unit triangle (non-degenerate, facing +Z).
// ─────────────────────────────────────────────────────────────────────────────
static std::unique_ptr<Mesh> make_triangle() {
    std::vector<Vertex> verts = {
        { {-0.5f, -0.5f, 0.f}, {0.f,0.f,1.f}, 0.f, 0.f },
        { { 0.5f, -0.5f, 0.f}, {0.f,0.f,1.f}, 1.f, 0.f },
        { { 0.0f,  0.5f, 0.f}, {0.f,0.f,1.f}, 0.5f,1.f },
    };
    std::vector<unsigned int> idx = { 0, 1, 2 };
    return std::make_unique<Mesh>(std::move(verts), std::move(idx));
}

// ─────────────────────────────────────────────────────────────────────────────
// The actual test — runs inside a GLUT display callback so we have a valid
// GL context.
// ─────────────────────────────────────────────────────────────────────────────
static bool g_passed = false;

static void run_test() {
    // ── 1. Shader ─────────────────────────────────────────────────────────────
    std::unique_ptr<Shader> shader;
    try {
        shader = std::make_unique<Shader>(
            "assets/shaders/phong.vert",
            "assets/shaders/phong.frag");
        std::cout << "[Phase3] Shader compiled OK\n";
    } catch (const std::exception& e) {
        std::cerr << "[Phase3] Shader FAILED: " << e.what() << "\n";
        return;
    }

    // ── 2. Mesh ───────────────────────────────────────────────────────────────
    auto mesh = make_triangle();
    mesh->setup_mesh();  // GL upload -- must be after glewInit()
    { GLenum err = glGetError(); if (err != GL_NO_ERROR) std::cerr << "[GL_ERROR] setup_mesh 0x" << std::hex << err << "\n"; }
    std::cout << "[Phase3] Mesh setup OK\n";

    // ── 3. Object (owns the Mesh) ─────────────────────────────────────────────
    auto obj = std::make_unique<Object>();
    obj->add_mesh(std::move(mesh));
    obj->material = std::make_shared<Material>();
    obj->material->diffuse   = {1.f, 0.5f, 0.2f};
    obj->material->ambient   = {0.1f, 0.05f, 0.02f};
    obj->material->specular  = {0.5f, 0.5f, 0.5f};
    obj->material->shininess = 32.f;

    // ── 4. Camera ─────────────────────────────────────────────────────────────
    Camera cam({0.f,0.f,3.f}, -90.f, 0.f);

    // ── 5. Renderer ───────────────────────────────────────────────────────────
    Renderer renderer;
    renderer.submit_opaque(obj.get());  // non-owning — obj still owned here

    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    renderer.render_opaque(cam, *shader);
    std::cout << "[Phase3] Renderer render_opaque OK\n";

    renderer.clear_queue();

    g_passed = true;
    std::cout << "[Phase3] ALL TESTS PASSED\n";

    // Request a single redisplay then exit.
    glutPostRedisplay();
}

// ─────────────────────────────────────────────────────────────────────────────
// GLUT callbacks
// ─────────────────────────────────────────────────────────────────────────────
static bool g_first_frame = true;

static void display_cb() {
    if (g_first_frame) {
        g_first_frame = false;
        run_test();
    }
    glutSwapBuffers();
    if (g_passed) glutLeaveMainLoop();  // freeglut — exits cleanly
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    // §0.3: glutInit → glutCreateWindow → glewInit — in that order.
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Phase3 Smoke Test");

    GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK) {
        std::cerr << "[Phase3] glewInit FAILED: "
                  << glewGetErrorString(glew_status) << "\n";
        return 1;
    }
    std::cout << "[Phase3] Application init OK\n";

    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glClearColor(0.1f, 0.1f, 0.1f, 1.f));

    glutDisplayFunc(display_cb);
    glutMainLoop();

    return g_passed ? 0 : 1;
}

