// tests/test_math.cpp
// ============================================================
//  Unit tests for the Math module -- zero third-party deps.
//  Build:  g++ -std=c++17 -I.. test_math.cpp ../math/matrix4.cpp -o test_math
//  Run:    ./test_math
// ============================================================

#include <cassert>
#include <cmath>
#include <cstdio>

#include "../math/vec2.h"
#include "../math/vec3.h"
#include "../math/vec4.h"
#include "../math/matrix4.h"
#include "../math/math_utils.h"

// -- Tolerance helper -----------------------------------------
static constexpr float k_eps = 1e-5f;

static bool feq(float a, float b, float eps = k_eps) {
    return std::abs(a - b) <= eps;
}

// -------------------------------------------------------------
//  Vec2 tests
// -------------------------------------------------------------
static void test_vec2() {
    Vec2 a{3.f, 4.f};
    assert(feq(a.length(), 5.f));

    Vec2 n = a.normalized();
    assert(feq(n.length(), 1.f));

    Vec2 b{1.f, 0.f};
    Vec2 c{0.f, 1.f};
    assert(feq(b.dot(c), 0.f));

    Vec2 sum = b + c;
    assert(feq(sum.x, 1.f) && feq(sum.y, 1.f));

    std::printf("[PASS] Vec2\n");
}

// -------------------------------------------------------------
//  Vec3 tests
// -------------------------------------------------------------
static void test_vec3_cross() {
    // x_hat x y_hat = z_hat
    Vec3 x_hat{1.f, 0.f, 0.f};
    Vec3 y_hat{0.f, 1.f, 0.f};
    Vec3 z_hat = x_hat.cross(y_hat);

    assert(feq(z_hat.x, 0.f));
    assert(feq(z_hat.y, 0.f));
    assert(feq(z_hat.z, 1.f));

    // y_hat x x_hat = -z_hat  (anti-commutative)
    Vec3 neg_z = y_hat.cross(x_hat);
    assert(feq(neg_z.z, -1.f));

    // Arbitrary: (1,2,3) x (4,5,6) = (-3, 6, -3)
    Vec3 u{1.f, 2.f, 3.f};
    Vec3 v{4.f, 5.f, 6.f};
    Vec3 w = u.cross(v);
    assert(feq(w.x, -3.f));
    assert(feq(w.y,  6.f));
    assert(feq(w.z, -3.f));

    // Cross product is perpendicular to both operands
    assert(feq(w.dot(u), 0.f));
    assert(feq(w.dot(v), 0.f));

    // normalized() length == 1
    Vec3 n = Vec3{3.f, 4.f, 0.f}.normalized();
    assert(feq(n.length(), 1.f));

    std::printf("[PASS] Vec3 cross product\n");
}

// -------------------------------------------------------------
//  Vec4 tests
// -------------------------------------------------------------
static void test_vec4() {
    Vec4 a{1.f, 2.f, 3.f, 4.f};
    Vec4 b = a * 2.f;
    assert(feq(b.w, 8.f));

    Vec4 c = a + Vec4{0.f, 0.f, 0.f, 0.f};
    assert(c == a);

    std::printf("[PASS] Vec4\n");
}

// -------------------------------------------------------------
//  Matrix4 identity * Vec4
// -------------------------------------------------------------
static void test_matrix4_identity_vec4() {
    Matrix4 I = Matrix4::identity();
    Vec4 v{1.f, 2.f, 3.f, 4.f};
    Vec4 result = I * v;

    assert(feq(result.x, v.x));
    assert(feq(result.y, v.y));
    assert(feq(result.z, v.z));
    assert(feq(result.w, v.w));

    std::printf("[PASS] Matrix4 identity * Vec4\n");
}

// -------------------------------------------------------------
//  Matrix4::perspective -- element [2][2] correctness
// -------------------------------------------------------------
static void test_matrix4_perspective() {
    // Standard values
    const float fov    = 45.f;
    const float aspect = 16.f / 9.f;
    const float near_z = 0.1f;
    const float far_z  = 100.f;

    Matrix4 P = Matrix4::perspective(fov, aspect, near_z, far_z);

    // [2][2] = -(far + near) / (far - near)
    const float expected = -(far_z + near_z) / (far_z - near_z);
    assert(feq(P.at(2, 2), expected));

    // [3][2] == -1  (row 3, col 2) -- perspective divide
    assert(feq(P.at(3, 2), -1.f));

    // [2][3] = -2*far*near / (far - near)
    const float expected_23 = -(2.f * far_z * near_z) / (far_z - near_z);
    assert(feq(P.at(2, 3), expected_23));

    std::printf("[PASS] Matrix4 perspective [2][2] = %.6f (expected %.6f)\n",
                P.at(2, 2), expected);
}

// -------------------------------------------------------------
//  Matrix4::look_at -- forward vector is unit-length
// -------------------------------------------------------------
static void test_matrix4_look_at_forward() {
    Vec3 eye   {0.f, 0.f, 5.f};
    Vec3 center{0.f, 0.f, 0.f};
    Vec3 up    {0.f, 1.f, 0.f};

    Matrix4 V = Matrix4::look_at(eye, center, up);

    // The forward (camera -Z) row is row 2 (negated): at(2,0..2) = -f
    // f = normalize(center - eye) = (0,0,-1)
    // Row 2 = (0, 0, 1)  (stored as -f)
    Vec3 forward_row{V.at(2,0), V.at(2,1), V.at(2,2)};
    const float len = forward_row.length();
    assert(feq(len, 1.f));

    // For this setup: camera looks along -z, so f = (0,0,-1)
    // Row 2 should be (0, 0, 1)
    assert(feq(forward_row.x, 0.f));
    assert(feq(forward_row.y, 0.f));
    assert(feq(forward_row.z, 1.f));

    // Right row should also be unit-length
    Vec3 right_row{V.at(0,0), V.at(0,1), V.at(0,2)};
    assert(feq(right_row.length(), 1.f));

    // Up row unit-length
    Vec3 up_row{V.at(1,0), V.at(1,1), V.at(1,2)};
    assert(feq(up_row.length(), 1.f));

    std::printf("[PASS] Matrix4 look_at forward vector normalised\n");
}

// -------------------------------------------------------------
//  Matrix4 multiply: translate then identity == translate
// -------------------------------------------------------------
static void test_matrix4_multiply() {
    Matrix4 T = Matrix4::translate({1.f, 2.f, 3.f});
    Matrix4 I = Matrix4::identity();

    Matrix4 TI = T * I;
    assert(TI == T);

    Matrix4 IT = I * T;
    assert(IT == T);

    std::printf("[PASS] Matrix4 multiply (T*I == T)\n");
}

// -------------------------------------------------------------
//  Matrix4 transposed
// -------------------------------------------------------------
static void test_matrix4_transposed() {
    Matrix4 T = Matrix4::translate({5.f, 6.f, 7.f});
    Matrix4 Tt = T.transposed();

    // T(row,col) == Tt(col,row)
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            assert(feq(T.at(r, c), Tt.at(c, r)));

    std::printf("[PASS] Matrix4 transposed\n");
}

// -------------------------------------------------------------
//  Matrix4 inverse: M * M^-^1 ~= Identity
// -------------------------------------------------------------
static void test_matrix4_inverse() {
    Matrix4 T = Matrix4::translate({3.f, -2.f, 1.f});
    Matrix4 inv = T.inverse();
    Matrix4 I_approx = T * inv;
    Matrix4 I_ref    = Matrix4::identity();

    for (int i = 0; i < 16; ++i)
        assert(feq(I_approx.m[i], I_ref.m[i], 1e-4f));

    std::printf("[PASS] Matrix4 inverse (T * T^-1 ~= I)\n");
}

// -------------------------------------------------------------
//  math_utils tests
// -------------------------------------------------------------
static void test_math_utils() {
    assert(feq(to_radians(180.f), k_pi));
    assert(feq(to_degrees(k_pi), 180.f));

    assert(feq(lerp(0.f, 10.f, 0.5f), 5.f));
    assert(feq(lerp(0.f, 10.f, 0.f),  0.f));
    assert(feq(lerp(0.f, 10.f, 1.f), 10.f));

    assert(feq(clamp(5.f, 0.f, 10.f), 5.f));
    assert(feq(clamp(-1.f, 0.f, 10.f), 0.f));
    assert(feq(clamp(11.f, 0.f, 10.f), 10.f));

    std::printf("[PASS] math_utils (to_radians, lerp, clamp)\n");
}

// -------------------------------------------------------------
//  main
// -------------------------------------------------------------
int main() {
    std::printf("=== Solar System Mini Engine -- Math Tests ===\n");

    test_vec2();
    test_vec3_cross();
    test_vec4();
    test_matrix4_identity_vec4();
    test_matrix4_perspective();
    test_matrix4_look_at_forward();
    test_matrix4_multiply();
    test_matrix4_transposed();
    test_matrix4_inverse();
    test_math_utils();

    std::printf("=============================================\n");
    std::printf("All tests passed.\n");
    return 0;
}
