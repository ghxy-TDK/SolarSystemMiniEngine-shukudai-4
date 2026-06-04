#pragma once

#include "../math/vec3.h"

// A ray in world space defined by an origin point and a unit direction vector.
// Convention: direction should be normalised before use in intersection tests.
struct Ray {
    Vec3 origin;
    Vec3 direction;

    // Returns the point along the ray at parameter t: P(t) = origin + direction * t.
    Vec3 at(float t) const {
        return origin + direction * t;
    }
};
