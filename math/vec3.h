#pragma once
#include <cmath>

// ============================================================
//  Vec3 -- 3D vector, column-major convention, no GL dependency
// ============================================================

struct Vec3 {
    float x{0.f}, y{0.f}, z{0.f};

    // -- Construction -----------------------------------------
    constexpr Vec3() = default;
    constexpr Vec3(float x, float y, float z) : x{x}, y{y}, z{z} {}
    explicit constexpr Vec3(float v) : x{v}, y{v}, z{v} {}

    // -- Arithmetic -------------------------------------------
    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(float s)       const { return {x * s,   y * s,   z * s};   }
    constexpr Vec3 operator/(float s)       const { return {x / s,   y / s,   z / s};   }
    constexpr Vec3 operator-()              const { return {-x, -y, -z};                  }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s)       { x *= s;   y *= s;   z *= s;   return *this; }
    Vec3& operator/=(float s)       { x /= s;   y /= s;   z /= s;   return *this; }

    constexpr bool operator==(const Vec3& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
    constexpr bool operator!=(const Vec3& o) const { return !(*this == o); }

    // -- Geometry ---------------------------------------------
    [[nodiscard]] constexpr float dot(const Vec3& o) const {
        return x * o.x + y * o.y + z * o.z;
    }

    /// Right-hand cross product: this x o
    [[nodiscard]] constexpr Vec3 cross(const Vec3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    [[nodiscard]] float length()    const { return std::sqrt(dot(*this)); }
    [[nodiscard]] float length_sq() const { return dot(*this); }

    [[nodiscard]] Vec3 normalized() const {
        const float len = length();
        return (len > 1e-8f) ? (*this / len) : Vec3{};
    }
};

// Scalar left-multiply
inline constexpr Vec3 operator*(float s, const Vec3& v) { return v * s; }
