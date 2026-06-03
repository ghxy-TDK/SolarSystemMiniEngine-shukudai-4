#pragma once

// ============================================================
//  config/engine_config.h
//  编译期常量 — 所有值在编译时确定，不支持运行时修改。
// ============================================================

namespace EngineConfig {

// ── 窗口 ──────────────────────────────────────────────────
constexpr int         k_window_width      = 1280;       // 编译期常量
constexpr int         k_window_height     = 720;        // 编译期常量
constexpr const char* k_window_title      = "Solar System Mini Engine"; // 编译期常量

// ── 摄像机 ────────────────────────────────────────────────
constexpr float k_cam_speed       = 5.0f;   // 编译期常量  单位: 世界单位/秒
constexpr float k_cam_sensitivity = 0.1f;   // 编译期常量  鼠标灵敏度系数

// ── 投影 ──────────────────────────────────────────────────
constexpr float k_near_plane = 0.1f;        // 编译期常量
constexpr float k_far_plane  = 500.0f;      // 编译期常量
constexpr float k_fov_deg    = 45.0f;       // 编译期常量  垂直视角（度）

} // namespace EngineConfig
