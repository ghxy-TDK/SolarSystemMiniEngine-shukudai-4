#pragma once

// ============================================================
//  core/application.h
//  单例应用层：封装 GLUT 初始化、主循环和回调注册。
//
//  使用规则：
//    1. 调用 Application::instance().init() — 完成所有 GL 初始化。
//    2. 注册所需回调。
//    3. 调用 Application::instance().run() — 进入 GLUT 主循环。
//    4. 仅在 init() 返回后才可创建 Mesh / Shader / Texture。
// ============================================================

#include <functional>

class Application {
public:
    // 返回全局单例。
    static Application& instance();

    // 禁止拷贝与赋值。
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    // 初始化 GLUT 窗口、GLEW 及 GL 调试回调。
    // 内部顺序：glutInit → glutCreateWindow → glewInit → gl_debug::setup()
    // 返回 false 表示初始化失败（glewInit 错误等）。
    bool init(int argc, char** argv);

    // 进入 GLUT 主循环（永不返回）。
    void run();

    // ── 回调注册 ─────────────────────────────────────────
    // display callback：每帧绘制逻辑（清屏、渲染、swap buffer）。
    void set_display_callback(std::function<void()> cb);

    // keyboard callback：GLUT ASCII 按键事件。
    void set_keyboard_callback(std::function<void(unsigned char, int, int)> cb);

    // keyboard-up callback：按键释放事件（用于 Camera::set_key）。
    void set_keyboard_up_callback(std::function<void(unsigned char, int, int)> cb);

    // passive mouse motion（无按键移动，用于 FPS 鼠标旋转）。
    void set_motion_callback(std::function<void(int, int)> cb);

private:
    Application() = default;

    // ── GLUT 静态转发函数 ─────────────────────────────────
    static void glut_display();
    static void glut_keyboard(unsigned char key, int x, int y);
    static void glut_keyboard_up(unsigned char key, int x, int y);
    static void glut_passive_motion(int x, int y);

    // ── 用户回调存储 ──────────────────────────────────────
    std::function<void()>                       display_cb_;
    std::function<void(unsigned char, int, int)> keyboard_cb_;
    std::function<void(unsigned char, int, int)> keyboard_up_cb_;
    std::function<void(int, int)>               motion_cb_;

    bool initialized_ = false;
};
