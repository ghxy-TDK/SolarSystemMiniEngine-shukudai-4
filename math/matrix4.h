#pragma once
#include <array>
#include <cmath>
#include "vec3.h"
#include "vec4.h"

// ============================================================
//  Matrix4 -- 4x4 float matrix, COLUMN-MAJOR storage
//
//  m[col * 4 + row]  <-->  m[col][row]
//
//  Layout matches OpenGL / glUniformMatrix4fv directly.
//  Column vectors:   col0 = m[0..3], col1 = m[4..7], ...
// ============================================================

class Matrix4 {
public:
    // 16 floats, column-major: index = col*4 + row
    std::array<float, 16> m{};

    // -- Construction -----------------------------------------
    Matrix4() { m.fill(0.f); }

    /// Direct element access: (row, col), 0-indexed
    [[nodiscard]] float& at(int row, int col)             { return m[col * 4 + row]; }
    [[nodiscard]] float  at(int row, int col) const       { return m[col * 4 + row]; }

    /// Raw column-major pointer for glUniformMatrix4fv
    [[nodiscard]] const float* data() const { return m.data(); }

    // -- Static factories -------------------------------------

    static Matrix4 identity();

    /// Translation by (t.x, t.y, t.z)
    static Matrix4 translate(const Vec3& t);

    /// Rotation by deg degrees around axis (normalised inside)
    static Matrix4 rotate(float deg, const Vec3& axis);

    /// Non-uniform scale
    static Matrix4 scale(const Vec3& s);

    /// Symmetric perspective projection (right-hand, NDC depth [-1,1])
    static Matrix4 perspective(float fov_deg, float aspect, float near_z, float far_z);

    /// View matrix from eye, centre, and up hint
    static Matrix4 look_at(const Vec3& eye, const Vec3& center, const Vec3& up);

    // -- Operators --------------------------------------------

    Matrix4  operator* (const Matrix4& rhs) const;
    Matrix4& operator*=(const Matrix4& rhs);

    /// Transform a Vec4 by this matrix (M * v)
    Vec4 operator*(const Vec4& v) const;

    bool operator==(const Matrix4& rhs) const { return m == rhs.m; }
    bool operator!=(const Matrix4& rhs) const { return m != rhs.m; }

    // -- Utility ----------------------------------------------

    [[nodiscard]] Matrix4 transposed() const;

    /// Inverse via cofactor expansion; returns identity if singular
    [[nodiscard]] Matrix4 inverse() const;
};
