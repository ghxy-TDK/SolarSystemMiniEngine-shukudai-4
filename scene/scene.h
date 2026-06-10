#pragma once
#include "object.h"
#include "point_light.h"
#include "../particle/particle_system.h"
#include <memory>
#include <vector>

// ---------------------------------------------------------------------------
// Scene
//
// Central ownership container.  Owns all Objects, Lights, and
// ParticleSystems via unique_ptr (section 0.2).
//
// Renderer and other systems receive non-owning raw pointers obtained
// from the get_* view methods; they must not extend object lifetimes.
// ---------------------------------------------------------------------------
class Scene {
public:
    Scene()  = default;
    ~Scene() = default;

    // Non-copyable, non-movable to prevent accidental ownership transfer.
    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;

    // --- add ---
    void add_object(std::unique_ptr<Object> obj);
    void add_light(std::unique_ptr<PointLight> light);
    void add_particle_system(std::unique_ptr<ParticleSystem> ps);

    // --- non-owning views ---
    std::vector<Object*>         get_objects()          const;
    std::vector<PointLight*>     get_lights()           const;
    std::vector<ParticleSystem*> get_particle_systems() const;

    // --- update ---
    // Propagates delta_time to all ParticleSystems.
    // Other update logic (orbit animation) lives in SolarSystem (stage 7).
    void update(float delta_time);

    // --- inspection ---
    std::size_t object_count()          const { return objects_.size();          }
    std::size_t light_count()           const { return lights_.size();           }
    std::size_t particle_system_count() const { return particle_systems_.size(); }

private:
    std::vector<std::unique_ptr<Object>>         objects_;
    std::vector<std::unique_ptr<PointLight>>     lights_;
    std::vector<std::unique_ptr<ParticleSystem>> particle_systems_;
};
