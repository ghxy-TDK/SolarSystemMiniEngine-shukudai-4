// ============================================================
//  main_test.cpp
//  阶段 2 最小可运行验证：
//    Application::init → 创建 Camera → glutMainLoop → 清屏为深灰色
//
//  编译示例（Linux）：
//    g++ -std=c++17 main_test.cpp              \
//        core/application.cpp                  \
//        core/timer.cpp                        \
//        debug/gl_debug.cpp                    \
//        render/camera.cpp                     \
//        -I.                                   \
//        -lGL -lGLEW -lglut -o solar_test
// ============================================================

#include "core/application.h"
#include "core/timer.h"
#include "render/camera.h"
#include "math/vec3.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cstdio>

// ── 全局摄像机（init() 后构造，在 main 栈上分配）─────────
static Camera* g_camera = nullptr;

// 上一帧鼠标位置（用于计算增量）
static int g_last_mouse_x = 640;
static int g_last_mouse_y = 360;
static bool g_first_mouse = true;

// ── Display Callback ──────────────────────────────────────
static void on_display()
{
    Timer::tick();

    // 清屏为深灰色 (0x22, 0x22, 0x22)
    glClearColor(0.133f, 0.133f, 0.133f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 每帧更新摄像机位置（实际渲染阶段再传矩阵给 shader）
    if (g_camera)
        g_camera->process_keyboard(Timer::delta_time());

    // 此处可插入调试输出，确认矩阵计算正常
    // Matrix4 view = g_camera->get_view_matrix();
    // Matrix4 proj = g_camera->get_projection_matrix();
}

// ── Keyboard Callbacks ────────────────────────────────────
static void on_key_down(unsigned char key, int /*x*/, int /*y*/)
{
    if (key == 27) {          // ESC 退出
        std::fprintf(stdout, "[main_test] ESC pressed — exit.\n");
        glutLeaveMainLoop();
        return;
    }
    if (g_camera) g_camera->set_key(key, true);
}

static void on_key_up(unsigned char key, int /*x*/, int /*y*/)
{
    if (g_camera) g_camera->set_key(key, false);
}

// ── Mouse Motion Callback ─────────────────────────────────
static void on_mouse_motion(int x, int y)
{
    if (g_first_mouse) {
        g_last_mouse_x = x;
        g_last_mouse_y = y;
        g_first_mouse  = false;
        return;
    }
    float dx = static_cast<float>(x - g_last_mouse_x);
    float dy = static_cast<float>(y - g_last_mouse_y);
    g_last_mouse_x = x;
    g_last_mouse_y = y;

    if (g_camera) g_camera->process_mouse(dx, dy);
}

// ── main ──────────────────────────────────────────────────
int main(int argc, char** argv)
{
    // 1. 初始化（GL 上下文在此建立）
    if (!Application::instance().init(argc, argv)) {
        std::fprintf(stderr, "[main_test] Application::init() failed.\n");
        return 1;
    }

    // 2. init() 返回后才能创建使用 GL 资源的对象
    //    Camera 本身不持有 GL 资源，但仍遵循约定在此构造
    Camera camera({0.f, 0.f, 5.f}, -90.f, 0.f);
    g_camera = &camera;

    std::fprintf(stdout,
        "[main_test] Camera created at (0, 0, 5), yaw=-90, pitch=0.\n"
        "[main_test] WASD: move | Mouse: look | ESC: exit\n");

    // 3. 注册回调
    Application::instance().set_display_callback(on_display);
    Application::instance().set_keyboard_callback(on_key_down);
    Application::instance().set_keyboard_up_callback(on_key_up);
    Application::instance().set_motion_callback(on_mouse_motion);

    // 4. 进入主循环（永不返回，直到 ESC）
    Application::instance().run();

    return 0;
}
