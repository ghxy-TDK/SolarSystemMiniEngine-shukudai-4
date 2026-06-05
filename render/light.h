#pragma once

#include "../math/vec3.h"

class Shader;

// PointLight: a single positional light source.
// Owned by Scene via unique_ptr<PointLight>.
// Renderer holds a non-owning raw pointer during a frame.
class PointLight {
public:
    // --- data ---
    Vec3  position  = {0.0f, 0.0f, 0.0f};
    Vec3  color     = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;

    // --- construction ---
    PointLight() = default;

    PointLight(const Vec3& pos, const Vec3& col, float inten)
        : position(pos), color(col), intensity(inten) {}

    // apply() uploads u_point_light.* uniforms to the active shader.
    // The shader must already be bound (shader.use() called before this).
    void apply(Shader& shader) const;
};
