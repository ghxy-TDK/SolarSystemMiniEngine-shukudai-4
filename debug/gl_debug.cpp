// ============================================================
//  debug/gl_debug.cpp
// ============================================================

#include "gl_debug.h"

#include <GL/glew.h>

#include <cstdio>
#include <string>

// ── 内部：KHR_debug 回调 ──────────────────────────────────

static void GLAPIENTRY gl_message_callback(
    GLenum        source,
    GLenum        type,
    GLuint        id,
    GLenum        severity,
    GLsizei       /*length*/,
    const GLchar* message,
    const void*   /*userParam*/)
{
    // 过滤无关紧要的通知（部分驱动会产生大量 INFO 消息）
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    const char* src_str = [source]() -> const char* {
        switch (source) {
            case GL_DEBUG_SOURCE_API:             return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "Window System";
            case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
            case GL_DEBUG_SOURCE_THIRD_PARTY:     return "Third Party";
            case GL_DEBUG_SOURCE_APPLICATION:     return "Application";
            default:                              return "Other";
        }
    }();

    const char* type_str = [type]() -> const char* {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               return "ERROR";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "UNDEFINED BEHAVIOR";
            case GL_DEBUG_TYPE_PORTABILITY:         return "PORTABILITY";
            case GL_DEBUG_TYPE_PERFORMANCE:         return "PERFORMANCE";
            case GL_DEBUG_TYPE_MARKER:              return "MARKER";
            default:                                return "OTHER";
        }
    }();

    const char* sev_str = [severity]() -> const char* {
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:   return "HIGH";
            case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
            case GL_DEBUG_SEVERITY_LOW:    return "LOW";
            default:                       return "NOTIFICATION";
        }
    }();

    std::fprintf(stderr,
        "[GL DEBUG] src=%s  type=%s  sev=%s  id=%u\n  %s\n",
        src_str, type_str, sev_str, id, message);
}

// ── 公开接口 ──────────────────────────────────────────────

namespace gl_debug {

void setup()
{
    // 检查扩展可用性
    if (!GLEW_KHR_debug) {
        std::fprintf(stderr,
            "[gl_debug] WARNING: KHR_debug not available — "
            "GL debug output disabled.\n");
        return;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);   // 回调在 GL 调用同线程同步触发
    glDebugMessageCallback(gl_message_callback, nullptr);

    // 让所有类型和严重级别的消息都通过（NOTIFICATION 在回调内部过滤）
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
                          0, nullptr, GL_TRUE);

    std::fprintf(stderr, "[gl_debug] GL debug callback registered.\n");
}

bool check_gl_error(const char* location)
{
    bool had_error = false;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char* err_str = [err]() -> const char* {
            switch (err) {
                case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
                case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
                case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
                case GL_STACK_OVERFLOW:                return "GL_STACK_OVERFLOW";
                case GL_STACK_UNDERFLOW:               return "GL_STACK_UNDERFLOW";
                case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
                case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
                default:                               return "UNKNOWN";
            }
        }();
        std::fprintf(stderr, "[GL ERROR] %s — %s (0x%X)\n",
                     location, err_str, err);
        had_error = true;
    }
    return had_error;
}

} // namespace gl_debug
