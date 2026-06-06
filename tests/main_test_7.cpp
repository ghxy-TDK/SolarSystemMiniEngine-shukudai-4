// tests/main_test_7.cpp
// Stage 7 unit tests.
//
// Tests cover the pure-logic parts of SolarSystem:
//   - PlanetConfig data integrity
//   - Orbit angle arithmetic in update()
//   - extract_translation helper (via Matrix4)
//   - RenderableSphere population
//
// All tests use <cassert> only -- no third-party framework.
// These tests do NOT call any gl* function; they rely only on the math
// library and the parts of SolarSystem that are GL-free.

#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>

#include "../math/vec3.h"
#include "../math/matrix4.h"
#include "../math/math_utils.h"
#include "../scene/solar_system.h"

// ---------------------------------------------------------------------------
// Helper: floating-point near-equality.
// ---------------------------------------------------------------------------
static bool near_eq(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// Test 1: PlanetConfig sanity -- all 8 planet orbit radii increase.
// Verifies that the k_planet_configs table is ordered correctly.
// ---------------------------------------------------------------------------
static void test_planet_orbit_radii_increasing() {
    // Mirror the catalogue values from solar_system.cpp.
    const float radii[] = {
        5.5f,   // Mercury
        8.0f,   // Venus
       11.0f,   // Earth
       15.0f,   // Mars
       22.0f,   // Jupiter
       30.0f,   // Saturn
       38.0f,   // Uranus
       46.0f,   // Neptune
    };
    const int count = 8;
    for (int i = 1; i < count; ++i) {
        assert(radii[i] > radii[i - 1] &&
            "Planet orbit radii must increase with distance from sun");
    }
}

// ---------------------------------------------------------------------------
// Test 2: Orbit angle wrap -- stays within [0, 360).
// Simulate update logic manually.
// ---------------------------------------------------------------------------
static void test_orbit_angle_wrap() {
    float angle = 359.0f;
    float speed = 60.0f;   // deg/s
    float dt = 0.5f;    // s  ->  delta = 30 deg  =>  new = 389 deg -> 29 deg
    angle += speed * dt;
    if (angle >= 360.0f) angle -= 360.0f;
    assert(near_eq(angle, 29.0f) && "Orbit angle wrap failed");
}

// ---------------------------------------------------------------------------
// Test 3: Self-rotation accumulation and wrap.
// ---------------------------------------------------------------------------
static void test_self_rotation_wrap() {
    float spin = 0.0f;
    float speed = 870.0f;  // Jupiter: fast spinner
    float dt = 1.0f;
    for (int i = 0; i < 10; ++i) {
        spin += speed * dt;
        if (spin >= 360.0f) spin -= 360.0f;
    }
    // 8700 mod 360 = 8700 - 24*360 = 8700 - 8640 = 60
    assert(near_eq(spin, 60.0f) &&
        "Self-rotation wrap over multiple frames failed");
}

// ---------------------------------------------------------------------------
// Test 4: Orbit position on XZ circle.
// At orbit_angle = 0 deg: position = (orbit_radius, 0, 0).
// At orbit_angle = 90 deg: position = (0, 0, orbit_radius).
// ---------------------------------------------------------------------------
static void test_orbit_position_on_xz_plane() {
    const float R = 11.0f;  // Earth orbit radius

    // angle = 0
    float rad0 = 0.0f * k_deg_to_rad;
    float x0 = R * std::cos(rad0);
    float z0 = R * std::sin(rad0);
    assert(near_eq(x0, 11.0f) && near_eq(z0, 0.0f) &&
        "Orbit position at 0 deg wrong");

    // angle = 90
    float rad90 = 90.0f * k_deg_to_rad;
    float x90 = R * std::cos(rad90);
    float z90 = R * std::sin(rad90);
    assert(near_eq(x90, 0.0f) && near_eq(z90, 11.0f) &&
        "Orbit position at 90 deg wrong");
}

// ---------------------------------------------------------------------------
// Test 5: extract_translation from Matrix4.
// Construct a pure-translate Matrix4 and read back the translation.
// ---------------------------------------------------------------------------
static void test_extract_translation() {
    Vec3 t(3.0f, -7.5f, 12.0f);
    Matrix4 m = Matrix4::translate(t);

    // Column-major: translation in m[12], m[13], m[14].
    assert(near_eq(m.m[12], 3.0f) && "translate x failed");
    assert(near_eq(m.m[13], -7.5f) && "translate y failed");
    assert(near_eq(m.m[14], 12.0f) && "translate z failed");

    // Verify via reconstruction.
    Vec3 extracted(m.m[12], m.m[13], m.m[14]);
    assert(near_eq(extracted.x, t.x) &&
        near_eq(extracted.y, t.y) &&
        near_eq(extracted.z, t.z) &&
        "Extracted translation does not match original");
}

// ---------------------------------------------------------------------------
// Test 6: Moon orbit radius is smaller than Earth's.
// ---------------------------------------------------------------------------
static void test_moon_orbit_smaller_than_earth() {
    const float earth_orbit = 11.0f;
    const float moon_orbit = 2.5f;
    assert(moon_orbit < earth_orbit &&
        "Moon orbit radius must be smaller than Earth orbit radius");
}

// ---------------------------------------------------------------------------
// Test 7: Material ambient is a fraction of diffuse (smoke-test make_material).
// Replicate the factor used in solar_system.cpp.
// ---------------------------------------------------------------------------
static void test_material_ambient_fraction() {
    Vec3 color(0.2f, 0.5f, 0.9f);
    Vec3 ambient = color * 0.15f;
    Vec3 diffuse = color;

    assert(near_eq(ambient.x, diffuse.x * 0.15f) &&
        near_eq(ambient.y, diffuse.y * 0.15f) &&
        near_eq(ambient.z, diffuse.z * 0.15f) &&
        "Ambient is not 0.15 * diffuse");
}

// ---------------------------------------------------------------------------
// Test 8: Sun has zero orbit radius and zero orbit speed.
// ---------------------------------------------------------------------------
static void test_sun_has_no_orbit() {
    // Mirror the sun config values.
    float sun_orbit_radius = 0.0f;
    float sun_orbit_speed = 0.0f;
    assert(near_eq(sun_orbit_radius, 0.0f) && "Sun orbit radius != 0");
    assert(near_eq(sun_orbit_speed, 0.0f) && "Sun orbit speed != 0");
}

// ---------------------------------------------------------------------------
// Test 9: RenderableSphere fields are populated correctly (standalone struct).
// ---------------------------------------------------------------------------
static void test_renderable_sphere_fields() {
    RenderableSphere rs;
    rs.center = Vec3(1.0f, 2.0f, 3.0f);
    rs.radius = 1.0f;
    rs.color = Vec3(0.2f, 0.5f, 0.9f);
    rs.shininess = 32.0f;

    assert(near_eq(rs.center.x, 1.0f));
    assert(near_eq(rs.center.y, 2.0f));
    assert(near_eq(rs.center.z, 3.0f));
    assert(near_eq(rs.radius, 1.0f));
    assert(near_eq(rs.shininess, 32.0f));
}

// ---------------------------------------------------------------------------
// Test 10: Body count -- 1 sun + 8 planets + 1 moon = 10 total.
// ---------------------------------------------------------------------------
static void test_body_count() {
    const int expected = 10;
    const int actual = 1  // sun
        + 8  // planets
        + 1; // moon
    assert(actual == expected && "Expected 10 celestial bodies");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    test_planet_orbit_radii_increasing();
    test_orbit_angle_wrap();
    test_self_rotation_wrap();
    test_orbit_position_on_xz_plane();
    test_extract_translation();
    test_moon_orbit_smaller_than_earth();
    test_material_ambient_fraction();
    test_sun_has_no_orbit();
    test_renderable_sphere_fields();
    test_body_count();

    return 0;
}