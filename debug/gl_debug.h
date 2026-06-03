#pragma once

// ============================================================
//  debug/gl_debug.h
//  OpenGL 调试工具：Debug Callback、手动错误检查、GL_CHECK 宏。
//  依赖 GL 4.3+ KHR_debug 扩展（由 GLEW 提供）。
//  必须在 glewInit() 之后调用 setup_gl_debug()。
// ============================================================

namespace gl_debug {

// 注册 glDebugMessageCallback（需 OpenGL 4.3+ / KHR_debug）。
// 调试输出会被路由到标准错误流。
// 必须在 glewInit() 完成后、首次 GL 调用前执行。
void setup();

// 手动轮询 glGetError() 并将错误信息打印到 stderr。
// location: 调用点描述字符串（如 "Mesh::setup_mesh"）。
// 返回 true 表示存在至少一个错误。
bool check_gl_error(const char* location);

} // namespace gl_debug

// ── GL_CHECK 宏 ────────────────────────────────────────────
// 包装单条 GL 调用，出错时打印文件、行号及调用文本，然后继续。
// 示例: GL_CHECK(glBindVertexArray(vao_));
#define GL_CHECK(call)                                                   \
    do {                                                                 \
        (call);                                                          \
        gl_debug::check_gl_error(#call " @ " __FILE__ ":" +             \
                                 std::to_string(__LINE__));              \
    } while (0)
