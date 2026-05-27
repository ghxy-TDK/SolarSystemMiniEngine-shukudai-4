#pragma once
#include <cmath>

// ============================================================
//  math_utils.h -- Scalar utilities, no GL dependency
// ============================================================

// -- Constants ------------------------------------------------

inline constexpr float k_pi          = 3.14159265358979323846f;
inline constexpr float k_two_pi      = k_pi * 2.f;
inline constexpr float k_half_pi     = k_pi * 0.5f;
inline constexpr float k_inv_pi      = 1.f / k_pi;
inline constexpr float k_deg_to_rad  = k_pi / 180.f;
inline constexpr float k_rad_to_deg  = 180.f / k_pi;

// -- Conversions ----------------------------------------------

[[nodiscard]] inline constexpr float to_radians(float deg) noexcept {
    return deg * k_deg_to_rad;
}

[[nodiscard]] inline constexpr float to_degrees(float rad) noexcept {
    return rad * k_rad_to_deg;
}

// -- Interpolation --------------------------------------------

/// Linear interpolation: a + t*(b-a), t in [0,1]
[[nodiscard]] inline constexpr float lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

// -- Clamping -------------------------------------------------

[[nodiscard]] inline constexpr float clamp(float v, float lo, float hi) noexcept {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

[[nodiscard]] inline constexpr float saturate(float v) noexcept {
    return clamp(v, 0.f, 1.f);
}

// -- Misc -----------------------------------------------------

[[nodiscard]] inline float wrap_angle(float rad) noexcept {
    // Bring rad into (-pi, pi]
    rad = std::fmod(rad, k_two_pi);
    if (rad >  k_pi) rad -= k_two_pi;
    if (rad < -k_pi) rad += k_two_pi;
    return rad;
}

/// Nearly-equal comparison for floats
[[nodiscard]] inline bool approx_equal(float a, float b,
                                       float eps = 1e-5f) noexcept {
    return std::abs(a - b) <= eps;
}
