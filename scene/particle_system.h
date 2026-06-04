#pragma once

// ---------------------------------------------------------------------------
// ParticleSystem stub.
// Full implementation comes in stage 9.
// Must be a complete type in scene.h so unique_ptr can call its destructor.
// ---------------------------------------------------------------------------
class ParticleSystem {
public:
    virtual void update(float /*delta_time*/) {}
    virtual ~ParticleSystem() = default;
};
