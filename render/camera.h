#pragma once

// ============================================================
//  render/camera.h
//  第一人称 FPS 摄像机：WASD 移动 + 鼠标旋转。
//
//  典型用法：
//    Camera cam({0,0,5}, -90.f, 0.f);
//    // 在 keyboard callback 中:
//    cam.set_key(key, true);
//    // 在 keyboard-up callback 中:
//    cam.set_key(key, false);
//    // 在 display callback 中:
//    cam.process_keyboard(Timer::delta_time());
//    shader.set_mat4("u_view",       cam.get_view_matrix());
//    shader.set_mat4("u_projection", cam.get_projection_matrix());
// ============================================================

#include "../math/vec3.h"
#include "../math/matrix4.h"

class Camera {
public:
    // pos       : 初始世界坐标
    // yaw_deg   : 水平旋转角（度），-90 为朝向 -Z 轴
    // pitch_deg : 垂直旋转角（度），0 为水平
    Camera(Vec3 pos, float yaw_deg, float pitch_deg);

    // ── 矩阵 ──────────────────────────────────────────────
    // 返回 look-at 视图矩阵（u_view）。
    Matrix4 get_view_matrix() const;

    // 返回透视投影矩阵（u_projection），参数来自 EngineConfig。
    Matrix4 get_projection_matrix() const;

    // ── 每帧更新 ──────────────────────────────────────────
    // 根据当前按键状态移动摄像机；速度来自 EngineConfig::k_cam_speed。
    void process_keyboard(float delta_time);

    // 响应鼠标增量旋转；灵敏度来自 EngineConfig::k_cam_sensitivity。
    // dx > 0 向右，dy > 0 向下（GLUT 坐标系）。
    void process_mouse(float dx, float dy);

    // ── 按键状态 ──────────────────────────────────────────
    // 供 GLUT keyboard / keyboard-up 回调调用。
    // 支持的键：'w','a','s','d'（大小写均可）。
    void set_key(unsigned char key, bool down);

    // ── 查询 ──────────────────────────────────────────────
    Vec3 position() const { return position_; }
    Vec3 front()    const { return front_;    }

private:
    void update_vectors();   // 由 yaw_/pitch_ 重算 front_/right_/up_

    Vec3  position_;
    Vec3  front_;
    Vec3  right_;
    Vec3  up_;
    Vec3  world_up_ = {0.f, 1.f, 0.f};

    float yaw_;    // 度
    float pitch_;  // 度，钳制在 ±89°

    // 按键状态（同时支持大小写）
    bool key_w_ = false;
    bool key_s_ = false;
    bool key_a_ = false;
    bool key_d_ = false;
};
