#pragma once
#include "../math/vec3.h"

// ---------------------------------------------------------------------------
// PointLight stub.
// Full implementation comes in a later stage.
// The struct must be complete (not just forward-declared) wherever
// unique_ptr<PointLight> is destroyed, i.e. in scene.h's destructor.
// ---------------------------------------------------------------------------
struct PointLight {
    Vec3  position  = {0.f, 0.f, 0.f};
    Vec3  color     = {1.f, 1.f, 1.f};
    float intensity = 1.f;

    virtual ~PointLight() = default;
};
