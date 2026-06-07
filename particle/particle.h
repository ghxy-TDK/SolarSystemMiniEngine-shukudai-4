#pragma once

#include "../math/vec3.h"
#include "../math/vec4.h"

// Plain data struct representing one particle.
// All fields are public; ParticleSystem owns and mutates them directly.
struct Particle {
    Vec3  position;
    Vec3  velocity;
    float life;      // remaining lifetime in seconds
    float max_life;  // total lifetime at spawn, used to derive alpha fade
    float size;      // point size in pixels at spawn
    Vec4  color;     // rgba, alpha channel drives fade-out
};
