#pragma once

// scene/solar_system.h
// Stage 7 -- SolarSystem core logic.
// Builds all celestial bodies into a Scene and advances their orbits each frame.

#include <vector>
#include <string>
#include "../math/vec3.h"
#include "../raytracing/intersection.h"  // for RenderableSphere (defined in snapshot D)

// Forward declarations -- avoid pulling in heavy headers here.
class Scene;
class Object;
class Transform;
class Renderer;
class Shader;

// ---------------------------------------------------------------------------
// PlanetConfig
// Static, per-planet parameters set at construction time.
// ---------------------------------------------------------------------------
struct PlanetConfig {
    std::string name;

    float orbit_radius;        // distance from parent, world units
    float orbit_speed;         // degrees per second (revolution)
    float self_rotation_speed; // degrees per second (spin)
    float radius;              // sphere radius, world units

    Vec3 color;                // used as diffuse color
};

// ---------------------------------------------------------------------------
// PlanetRecord
// Runtime bookkeeping for one celestial body (planet or moon).
// SolarSystem does NOT own the Object -- Scene does.
// ---------------------------------------------------------------------------
struct PlanetRecord {
    PlanetConfig config;

    float orbit_angle_deg;      // current revolution angle
    float self_rotation_deg;    // current spin angle

    Object*    object    = nullptr;  // non-owning -- Scene owns the Object
    Transform* transform = nullptr;  // non-owning -- points into object->transform
    Object*    orbit_obj = nullptr;  // non-owning -- the Orbit ring Object
};

// ---------------------------------------------------------------------------
// RenderableSphere  (re-declared here for this module; canonical def in
// raytracing/intersection.h or scene/solar_system.h -- whichever is primary)
// ---------------------------------------------------------------------------
#ifndef RENDERABLE_SPHERE_DEFINED
struct RenderableSphere {
    Vec3  center;
    float radius;
    Vec3  color;
    float shininess;
};
#endif

// ---------------------------------------------------------------------------
// SolarSystem
// ---------------------------------------------------------------------------
class SolarSystem {
public:
    SolarSystem();

    // Create all Objects/Meshes and inject them into scene.
    // Must be called AFTER glewInit() (GL context must be active).
    // sun_transform_out: if non-null, receives a non-owning pointer to the
    // sun's Transform so callers can use it as a scene anchor.
    void build(Scene& scene);

    // Advance orbit and self-rotation angles by delta_time seconds.
    void update(float delta_time);

    // Submit all orbit-ring Objects to the renderer's debug queue.
    void submit_orbits(Renderer& renderer) const;

    // Return world-space sphere data for every body (used by ray-tracer).
    // Extracts translation from Transform::get_world_matrix().
    std::vector<RenderableSphere> get_scene_data_for_raytracing() const;

    void submit_debug_objects(Renderer& r);

private:
    // Build one planet/moon record and add its Object to scene.
    // parent_transform: the Transform this body orbits (null for the sun).
    PlanetRecord build_body(Scene& scene,
                            const PlanetConfig& cfg,
                            Transform* parent_transform);

    // Build an Orbit ring Object and add it to scene.
    // parent_transform: orbit ring inherits the same parent as the planet.
    Object* build_orbit(Scene& scene,
                        float orbit_radius,
                        Transform* parent_transform);

    // All bodies: [0]=sun, [1..8]=planets, [9]=moon.
    std::vector<PlanetRecord> bodies_;

    // Non-owning convenience pointers for parent binding.
    Transform* sun_transform_   = nullptr;
    Transform* earth_transform_ = nullptr;
};

