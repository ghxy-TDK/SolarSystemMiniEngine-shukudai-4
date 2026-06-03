// ============================================================
//  core/application.cpp
// ============================================================

#include "application.h"

#include "../config/engine_config.h"
#include "../debug/gl_debug.h"

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cstdio>
#include <cstdlib>

// ── 单例 ─────────────────────────────────────────────────

Application& Application::instance()
{
    static Application s_instance;
    return s_instance;
}

// ── 初始化 ────────────────────────────────────────────────

bool Application::init(int argc, char** argv)
{
    if (initialized_) return true;

    // 1. GLUT 窗口
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(EngineConfig::k_window_width,
                       EngineConfig::k_window_height);
    glutCreateWindow(EngineConfig::k_window_title);

    // 2. GLEW 扩展
    glewExperimental = GL_TRUE;
    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK) {
        std::fprintf(stderr, "[Application] glewInit failed: %s\n",
                     glewGetErrorString(glew_err));
        return false;
    }

    // 3. GL Debug 回调（需 GL 4.3+，内部会检查扩展）
    gl_debug::setup();

    // 4. 注册 GLUT 静态回调（转发到实例成员函数）
    glutDisplayFunc(glut_display);
    glutKeyboardFunc(glut_keyboard);
    glutKeyboardUpFunc(glut_keyboard_up);
    glutPassiveMotionFunc(glut_passive_motion);

    // 基础 GL 状态
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initialized_ = true;
    std::fprintf(stdout, "[Application] Initialized. GL %s | GLSL %s\n",
                 glGetString(GL_VERSION),
                 glGetString(GL_SHADING_LANGUAGE_VERSION));
    return true;
}

// ── 主循环 ────────────────────────────────────────────────

void Application::run()
{
    glutMainLoop();
}

// ── 回调注册 ──────────────────────────────────────────────

void Application::set_display_callback(std::function<void()> cb)
{
    display_cb_ = std::move(cb);
}

void Application::set_keyboard_callback(
    std::function<void(unsigned char, int, int)> cb)
{
    keyboard_cb_ = std::move(cb);
}

void Application::set_keyboard_up_callback(
    std::function<void(unsigned char, int, int)> cb)
{
    keyboard_up_cb_ = std::move(cb);
}

void Application::set_motion_callback(std::function<void(int, int)> cb)
{
    motion_cb_ = std::move(cb);
}

// ── GLUT 静态转发 ─────────────────────────────────────────

void Application::glut_display()
{
    auto& app = instance();
    if (app.display_cb_) app.display_cb_();
    glutSwapBuffers();
    glutPostRedisplay();   // 持续渲染
}

void Application::glut_keyboard(unsigned char key, int x, int y)
{
    auto& app = instance();
    if (app.keyboard_cb_) app.keyboard_cb_(key, x, y);
}

void Application::glut_keyboard_up(unsigned char key, int x, int y)
{
    auto& app = instance();
    if (app.keyboard_up_cb_) app.keyboard_up_cb_(key, x, y);
}

void Application::glut_passive_motion(int x, int y)
{
    auto& app = instance();
    if (app.motion_cb_) app.motion_cb_(x, y);
}
