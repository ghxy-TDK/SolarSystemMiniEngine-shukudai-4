// tests/main_test_6.cpp
// Stage 6 entry point + smoke tests for PointLight.
// Uses the real project headers -- no stubs, no duplicate symbols.
// GL-dependent calls (Shader, draw) are NOT tested here;
// they are exercised by the live render path each frame.

#include <cassert>
#include <cmath>
#include <iostream>

#include "../render/light.h"
#include "../math/vec3.h"

static bool feq(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static void test_default_constructor() {
    PointLight light;
    assert(feq(light.position.x, 0.0f));
    assert(feq(light.position.y, 0.0f));
    assert(feq(light.position.z, 0.0f));
    assert(feq(light.color.x, 1.0f));
    assert(feq(light.color.y, 1.0f));
    assert(feq(light.color.z, 1.0f));
    assert(feq(light.intensity, 1.0f));
    std::cout << "PASS test_default_constructor\n";
}

static void test_value_constructor() {
    Vec3 pos = { 10.0f, 20.0f, 30.0f };
    Vec3 col = { 0.5f,  0.8f,  1.0f };
    PointLight light(pos, col, 2.5f);
    assert(feq(light.position.x, 10.0f));
    assert(feq(light.position.y, 20.0f));
    assert(feq(light.position.z, 30.0f));
    assert(feq(light.color.x, 0.5f));
    assert(feq(light.color.y, 0.8f));
    assert(feq(light.color.z, 1.0f));
    assert(feq(light.intensity, 2.5f));
    std::cout << "PASS test_value_constructor\n";
}

static void test_field_mutation() {
    PointLight light;
    light.position = { 5.0f, 0.0f, 0.0f };
    light.color = { 1.0f, 0.0f, 0.0f };
    light.intensity = 0.5f;
    assert(feq(light.position.x, 5.0f));
    assert(feq(light.color.x, 1.0f));
    assert(feq(light.intensity, 0.5f));
    std::cout << "PASS test_field_mutation\n";
}

// NOTE: apply() uploads uniforms to a live Shader and requires a GL
// context. It is tested implicitly every frame via Renderer::render_opaque.

int main() {
    std::cout << "=== Stage 6: PointLight tests ===\n";
    test_default_constructor();
    test_value_constructor();
    test_field_mutation();
    std::cout << "All Stage 6 tests passed.\n";
    return 0;
}