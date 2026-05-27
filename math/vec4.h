#pragma once
#include <cmath>

// ============================================================
//  Vec4 -- 4D vector (homogeneous coords), no GL dependency
// ============================================================

struct Vec4 {
    float x{0.f}, y{0.f}, z{0.f}, w{0.f};

    // -- Construction -----------------------------------------
    constexpr Vec4() = default;
    constexpr Vec4(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {}
    explicit constexpr Vec4(float v) : x{v}, y{v}, z{v}, w{v} {}

    // -- Arithmetic -------------------------------------------
    constexpr Vec4 operator+(const Vec4& o) const {
        return {x + o.x, y + o.y, z + o.z, w + o.w};
    }
    constexpr Vec4 operator-(const Vec4& o) const {
        return {x - o.x, y - o.y, z - o.z, w - o.w};
    }
    constexpr Vec4 operator*(float s) const { return {x * s, y * s, z * s, w * s}; }
    constexpr Vec4 operator/(float s) const { return {x / s, y / s, z / s, w / s}; }
    constexpr Vec4 operator-()        const { return {-x, -y, -z, -w};              }

    Vec4& operator+=(const Vec4& o) {
        x += o.x; y += o.y; z += o.z; w += o.w; return *this;
    }
    Vec4& operator-=(const Vec4& o) {
        x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this;
    }
    Vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    Vec4& operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }

    constexpr bool operator==(const Vec4& o) const {
        return x == o.x && y == o.y && z == o.z && w == o.w;
    }
    constexpr bool operator!=(const Vec4& o) const { return !(*this == o); }

    // -- Geometry ---------------------------------------------
    [[nodiscard]] constexpr float dot(const Vec4& o) const {
        return x * o.x + y * o.y + z * o.z + w * o.w;
    }
    [[nodiscard]] float length()    const { return std::sqrt(dot(*this)); }
    [[nodiscard]] float length_sq() const { return dot(*this); }

    [[nodiscard]] Vec4 normalized() const {
        const float len = length();
        return (len > 1e-8f) ? (*this / len) : Vec4{};
    }
};

// Scalar left-multiply
inline constexpr Vec4 operator*(float s, const Vec4& v) { return v * s; }
