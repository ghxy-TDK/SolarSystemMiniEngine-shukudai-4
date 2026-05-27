#pragma once
#include <cmath>

// ============================================================
//  Vec2 -- 2D vector, column-major convention, no GL dependency
// ============================================================

struct Vec2 {
    float x{0.f}, y{0.f};

    // -- Construction -----------------------------------------
    constexpr Vec2() = default;
    constexpr Vec2(float x, float y) : x{x}, y{y} {}
    explicit constexpr Vec2(float v) : x{v}, y{v} {}

    // -- Arithmetic -------------------------------------------
    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float s)       const { return {x * s,   y * s};   }
    constexpr Vec2 operator/(float s)       const { return {x / s,   y / s};   }
    constexpr Vec2 operator-()              const { return {-x, -y};            }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)       { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(float s)       { x /= s;   y /= s;   return *this; }

    constexpr bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2& o) const { return !(*this == o);         }

    // -- Geometry ---------------------------------------------
    [[nodiscard]] constexpr float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    [[nodiscard]] float length()      const { return std::sqrt(dot(*this)); }
    [[nodiscard]] float length_sq()   const { return dot(*this); }

    [[nodiscard]] Vec2 normalized() const {
        const float len = length();
        return (len > 1e-8f) ? (*this / len) : Vec2{};
    }
};

// Scalar left-multiply
inline constexpr Vec2 operator*(float s, const Vec2& v) { return v * s; }
