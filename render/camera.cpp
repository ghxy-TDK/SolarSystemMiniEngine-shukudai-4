// ============================================================
//  render/camera.cpp
// ============================================================

#include "camera.h"

#include "../config/engine_config.h"

#include <cmath>
#include <algorithm>

// 角度 → 弧度
static constexpr float k_pi     = 3.14159265358979323846f;
static constexpr float k_deg2rad = k_pi / 180.0f;

// ── 构造 ──────────────────────────────────────────────────

Camera::Camera(Vec3 pos, float yaw_deg, float pitch_deg)
    : position_(pos)
    , yaw_(yaw_deg)
    , pitch_(pitch_deg)
{
    update_vectors();
}

// ── 矩阵 ──────────────────────────────────────────────────

Matrix4 Camera::get_view_matrix() const
{
    Vec3 center = {
        position_.x + front_.x,
        position_.y + front_.y,
        position_.z + front_.z
    };
    return Matrix4::look_at(position_, center, up_);
}

Matrix4 Camera::get_projection_matrix() const
{
    float aspect = static_cast<float>(EngineConfig::k_window_width) /
                   static_cast<float>(EngineConfig::k_window_height);
    return Matrix4::perspective(
        EngineConfig::k_fov_deg,
        aspect,
        EngineConfig::k_near_plane,
        EngineConfig::k_far_plane);
}

// ── 每帧更新 ──────────────────────────────────────────────

void Camera::process_keyboard(float delta_time)
{
    float velocity = EngineConfig::k_cam_speed * delta_time;

    if (key_w_) {
        position_.x += front_.x * velocity;
        position_.y += front_.y * velocity;
        position_.z += front_.z * velocity;
    }
    if (key_s_) {
        position_.x -= front_.x * velocity;
        position_.y -= front_.y * velocity;
        position_.z -= front_.z * velocity;
    }
    if (key_a_) {
        position_.x -= right_.x * velocity;
        position_.y -= right_.y * velocity;
        position_.z -= right_.z * velocity;
    }
    if (key_d_) {
        position_.x += right_.x * velocity;
        position_.y += right_.y * velocity;
        position_.z += right_.z * velocity;
    }
}

void Camera::process_mouse(float dx, float dy)
{
    yaw_   += dx * EngineConfig::k_cam_sensitivity;
    pitch_ -= dy * EngineConfig::k_cam_sensitivity;   // GLUT y 轴向下
    // 钳制 pitch，避免万向锁
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    update_vectors();
}

// ── 按键状态 ──────────────────────────────────────────────

void Camera::set_key(unsigned char key, bool down)
{
    switch (key) {
        case 'w': case 'W': key_w_ = down; break;
        case 's': case 'S': key_s_ = down; break;
        case 'a': case 'A': key_a_ = down; break;
        case 'd': case 'D': key_d_ = down; break;
        default: break;
    }
}

// ── 内部：向量重算 ────────────────────────────────────────

void Camera::update_vectors()
{
    float yr = yaw_   * k_deg2rad;
    float pr = pitch_ * k_deg2rad;

    front_ = {
        std::cos(yr) * std::cos(pr),
        std::sin(pr),
        std::sin(yr) * std::cos(pr)
    };
    front_ = front_.normalized();

    right_ = front_.cross(world_up_).normalized();
    up_    = right_.cross(front_).normalized();
}
