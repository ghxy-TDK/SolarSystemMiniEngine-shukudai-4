#pragma once

#include <vector>
#include "../particle/particle.h"
#include "../math/vec3.h"

// Forward declarations to avoid pulling in GL / full class headers here.
class Camera;
class Shader;

// CPU-simulated, GL_POINTS-rendered particle emitter.
//
// Ownership: Scene holds this via unique_ptr<ParticleSystem>.
// GL resources (vao_, vbo_) are created in the constructor and
// released in the destructor; construction must occur after glewInit().
class ParticleSystem {
public:
    // max_particles: hard cap on simultaneous live particles.
    // emit_pos:      world-space position of the emitter.
    ParticleSystem(int max_particles, Vec3 emit_pos);
    ~ParticleSystem();

    // Non-copyable, non-movable (owns GL handles).
    ParticleSystem(const ParticleSystem&)            = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // Advance simulation by dt seconds.
    // Spawns new particles to fill dead slots and integrates live ones.
    void update(float dt);

    // Upload active particles to the GPU and issue a single GL_POINTS draw.
    // particle_shader must already be bound (use() called) by the caller.
    void draw(const Camera& cam, Shader& particle_shader) const;

    // SolarSystem calls this each frame to track a planet/sun position.
    void set_emit_position(Vec3 pos);

private:
    // --- simulation state ---
    std::vector<Particle> particles_;  // fixed-capacity pool
    int    max_particles_;
    Vec3   emit_pos_;

    // --- GL handles ---
    unsigned int vao_;
    unsigned int vbo_;  // stores vec3 positions of active particles

    // --- per-emitter appearance knobs ---
    // These are uniform across the whole system for simplicity.
    float emit_rate_;          // particles spawned per second
    float spawn_accumulator_;  // fractional spawn debt carried between frames

    // Helpers
    void      init_gl();          // called once from constructor
    void      spawn_particle(Particle& p) const;  // fills p with fresh values
    void      upload_to_gpu() const;  // maps VBO and writes live positions
    int       find_dead_slot() const; // returns index of first life<=0 particle
};
