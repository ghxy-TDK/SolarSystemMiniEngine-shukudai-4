#pragma once

#include <limits>
#include "../math/vec3.h"

// Records the result of a ray–primitive intersection test.
// Initialised to a "miss" state; intersection routines populate fields on a hit.
struct Intersection {
    // True when the ray hit a surface.
    bool hit = false;

    // Ray parameter t at the hit point. Initialised to +∞ so that any real hit
    // always beats the default, enabling straightforward closest-hit tracking.
    float t = std::numeric_limits<float>::max();

    // World-space hit point.
    Vec3 point;

    // Outward unit surface normal at the hit point.
    Vec3 normal;

    // Index into the scene's material/sphere array; -1 means unset.
    int material_id = -1;
};
