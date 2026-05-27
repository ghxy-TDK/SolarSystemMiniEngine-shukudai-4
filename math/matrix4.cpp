#include "matrix4.h"
#include "math_utils.h"
#include <cstring>  // std::memset
#include <cmath>

// -------------------------------------------------------------
//  Internal helpers
// -------------------------------------------------------------

// Convenience: set element at (row, col)
static inline void set(Matrix4& mat, int row, int col, float val) {
    mat.m[col * 4 + row] = val;
}
static inline float get(const Matrix4& mat, int row, int col) {
    return mat.m[col * 4 + row];
}

// -------------------------------------------------------------
//  Static factories
// -------------------------------------------------------------

Matrix4 Matrix4::identity() {
    Matrix4 r;
    r.at(0,0) = r.at(1,1) = r.at(2,2) = r.at(3,3) = 1.f;
    return r;
}

Matrix4 Matrix4::translate(const Vec3& t) {
    Matrix4 r = identity();
    r.at(0,3) = t.x;
    r.at(1,3) = t.y;
    r.at(2,3) = t.z;
    return r;
}

Matrix4 Matrix4::rotate(float deg, const Vec3& axis) {
    const float rad = to_radians(deg);
    const float c   = std::cos(rad);
    const float s   = std::sin(rad);
    const float one_minus_c = 1.f - c;

    const Vec3 n = axis.normalized();
    const float x = n.x, y = n.y, z = n.z;

    Matrix4 r;
    // Row 0
    r.at(0,0) = c + x*x*one_minus_c;
    r.at(0,1) = x*y*one_minus_c - z*s;
    r.at(0,2) = x*z*one_minus_c + y*s;
    r.at(0,3) = 0.f;
    // Row 1
    r.at(1,0) = y*x*one_minus_c + z*s;
    r.at(1,1) = c + y*y*one_minus_c;
    r.at(1,2) = y*z*one_minus_c - x*s;
    r.at(1,3) = 0.f;
    // Row 2
    r.at(2,0) = z*x*one_minus_c - y*s;
    r.at(2,1) = z*y*one_minus_c + x*s;
    r.at(2,2) = c + z*z*one_minus_c;
    r.at(2,3) = 0.f;
    // Row 3
    r.at(3,0) = 0.f;
    r.at(3,1) = 0.f;
    r.at(3,2) = 0.f;
    r.at(3,3) = 1.f;
    return r;
}

Matrix4 Matrix4::scale(const Vec3& s) {
    Matrix4 r;
    r.at(0,0) = s.x;
    r.at(1,1) = s.y;
    r.at(2,2) = s.z;
    r.at(3,3) = 1.f;
    return r;
}

Matrix4 Matrix4::perspective(float fov_deg, float aspect, float near_z, float far_z) {
    // Right-hand coordinate system, depth maps to [-1, 1] (OpenGL default)
    const float half_fov = to_radians(fov_deg) * 0.5f;
    const float f        = 1.f / std::tan(half_fov);   // cot(half_fov)
    const float range    = far_z - near_z;

    Matrix4 r;
    r.at(0,0) =  f / aspect;
    r.at(1,1) =  f;
    r.at(2,2) = -(far_z + near_z) / range;          // [2][2]
    r.at(3,2) = -1.f;                                // maps z -> -w
    r.at(2,3) = -(2.f * far_z * near_z) / range;
    // r.at(3,3) stays 0
    return r;
}

Matrix4 Matrix4::look_at(const Vec3& eye, const Vec3& center, const Vec3& up) {
    // Right-hand look-at (standard OpenGL convention)
    const Vec3 f = (center - eye).normalized();   // forward  (-z axis)
    const Vec3 r = f.cross(up).normalized();      // right     (x axis)
    const Vec3 u = r.cross(f);                    // true up   (y axis)

    Matrix4 mat;
    // Rotation part (rows are basis vectors of camera space)
    mat.at(0,0) =  r.x;  mat.at(0,1) =  r.y;  mat.at(0,2) =  r.z;
    mat.at(1,0) =  u.x;  mat.at(1,1) =  u.y;  mat.at(1,2) =  u.z;
    mat.at(2,0) = -f.x;  mat.at(2,1) = -f.y;  mat.at(2,2) = -f.z;
    mat.at(3,3) =  1.f;

    // Translation: -dot(basis, eye)
    mat.at(0,3) = -r.dot(eye);
    mat.at(1,3) = -u.dot(eye);
    mat.at(2,3) =  f.dot(eye);
    return mat;
}

// -------------------------------------------------------------
//  Operators
// -------------------------------------------------------------

Matrix4 Matrix4::operator*(const Matrix4& rhs) const {
    Matrix4 result;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.f;
            for (int k = 0; k < 4; ++k) {
                sum += at(row, k) * rhs.at(k, col);
            }
            result.at(row, col) = sum;
        }
    }
    return result;
}

Matrix4& Matrix4::operator*=(const Matrix4& rhs) {
    *this = *this * rhs;
    return *this;
}

Vec4 Matrix4::operator*(const Vec4& v) const {
    return {
        at(0,0)*v.x + at(0,1)*v.y + at(0,2)*v.z + at(0,3)*v.w,
        at(1,0)*v.x + at(1,1)*v.y + at(1,2)*v.z + at(1,3)*v.w,
        at(2,0)*v.x + at(2,1)*v.y + at(2,2)*v.z + at(2,3)*v.w,
        at(3,0)*v.x + at(3,1)*v.y + at(3,2)*v.z + at(3,3)*v.w
    };
}

// -------------------------------------------------------------
//  Utility
// -------------------------------------------------------------

Matrix4 Matrix4::transposed() const {
    Matrix4 r;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            r.at(row, col) = at(col, row);
    return r;
}

// 4x4 inverse via cofactor / adjugate method (Cramer's rule variant)
Matrix4 Matrix4::inverse() const {
    // Cofactor expansion -- standard 4x4 formula used in GLM
    const auto& a = m;  // column-major; a[col*4+row]

    float inv[16];

    inv[0]  =  a[5]*a[10]*a[15] - a[5]*a[11]*a[14] - a[9]*a[6]*a[15]
             + a[9]*a[7]*a[14]  + a[13]*a[6]*a[11]  - a[13]*a[7]*a[10];

    inv[4]  = -a[4]*a[10]*a[15] + a[4]*a[11]*a[14]  + a[8]*a[6]*a[15]
             - a[8]*a[7]*a[14]  - a[12]*a[6]*a[11]  + a[12]*a[7]*a[10];

    inv[8]  =  a[4]*a[9]*a[15]  - a[4]*a[11]*a[13]  - a[8]*a[5]*a[15]
             + a[8]*a[7]*a[13]  + a[12]*a[5]*a[11]  - a[12]*a[7]*a[9];

    inv[12] = -a[4]*a[9]*a[14]  + a[4]*a[10]*a[13]  + a[8]*a[5]*a[14]
             - a[8]*a[6]*a[13]  - a[12]*a[5]*a[10]  + a[12]*a[6]*a[9];

    inv[1]  = -a[1]*a[10]*a[15] + a[1]*a[11]*a[14]  + a[9]*a[2]*a[15]
             - a[9]*a[3]*a[14]  - a[13]*a[2]*a[11]  + a[13]*a[3]*a[10];

    inv[5]  =  a[0]*a[10]*a[15] - a[0]*a[11]*a[14]  - a[8]*a[2]*a[15]
             + a[8]*a[3]*a[14]  + a[12]*a[2]*a[11]  - a[12]*a[3]*a[10];

    inv[9]  = -a[0]*a[9]*a[15]  + a[0]*a[11]*a[13]  + a[8]*a[1]*a[15]
             - a[8]*a[3]*a[13]  - a[12]*a[1]*a[11]  + a[12]*a[3]*a[9];

    inv[13] =  a[0]*a[9]*a[14]  - a[0]*a[10]*a[13]  - a[8]*a[1]*a[14]
             + a[8]*a[2]*a[13]  + a[12]*a[1]*a[10]  - a[12]*a[2]*a[9];

    inv[2]  =  a[1]*a[6]*a[15]  - a[1]*a[7]*a[14]   - a[5]*a[2]*a[15]
             + a[5]*a[3]*a[14]  + a[13]*a[2]*a[7]   - a[13]*a[3]*a[6];

    inv[6]  = -a[0]*a[6]*a[15]  + a[0]*a[7]*a[14]   + a[4]*a[2]*a[15]
             - a[4]*a[3]*a[14]  - a[12]*a[2]*a[7]   + a[12]*a[3]*a[6];

    inv[10] =  a[0]*a[5]*a[15]  - a[0]*a[7]*a[13]   - a[4]*a[1]*a[15]
             + a[4]*a[3]*a[13]  + a[12]*a[1]*a[7]   - a[12]*a[3]*a[5];

    inv[14] = -a[0]*a[5]*a[14]  + a[0]*a[6]*a[13]   + a[4]*a[1]*a[14]
             - a[4]*a[2]*a[13]  - a[12]*a[1]*a[6]   + a[12]*a[2]*a[5];

    inv[3]  = -a[1]*a[6]*a[11]  + a[1]*a[7]*a[10]   + a[5]*a[2]*a[11]
             - a[5]*a[3]*a[10]  - a[9]*a[2]*a[7]    + a[9]*a[3]*a[6];

    inv[7]  =  a[0]*a[6]*a[11]  - a[0]*a[7]*a[10]   - a[4]*a[2]*a[11]
             + a[4]*a[3]*a[10]  + a[8]*a[2]*a[7]    - a[8]*a[3]*a[6];

    inv[11] = -a[0]*a[5]*a[11]  + a[0]*a[7]*a[9]    + a[4]*a[1]*a[11]
             - a[4]*a[3]*a[9]   - a[8]*a[1]*a[7]    + a[8]*a[3]*a[5];

    inv[15] =  a[0]*a[5]*a[10]  - a[0]*a[6]*a[9]    - a[4]*a[1]*a[10]
             + a[4]*a[2]*a[9]   + a[8]*a[1]*a[6]    - a[8]*a[2]*a[5];

    float det = a[0]*inv[0] + a[1]*inv[4] + a[2]*inv[8] + a[3]*inv[12];

    if (std::abs(det) < 1e-8f) {
        return identity();   // singular -> return identity (graceful fallback)
    }

    det = 1.f / det;
    Matrix4 result;
    for (int i = 0; i < 16; ++i)
        result.m[i] = inv[i] * det;

    return result;
}
