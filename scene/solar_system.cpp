// scene/solar_system.cpp
// Stage 7 -- SolarSystem implementation.

#include "solar_system.h"

#include <cmath>
#include <memory>
#include <cassert>

// Project headers (paths relative to project root).
#include "../math/vec3.h"
#include "../math/matrix4.h"
#include "../math/math_utils.h"
#include "../scene/transform.h"
#include "../scene/object.h"
#include "../scene/scene.h"
#include "../render/material.h"
#include "../render/renderer.h"
#include "../geometry/sphere.h"
#include "../geometry/orbit.h"
#include "../geometry/mesh.h"

// ---------------------------------------------------------------------------
// Planet catalogue
// orbit_radius  orbit_speed  self_rotation_speed  radius  color(R,G,B)
// Speeds are in degrees/second.  Radii are in scene units.
// ---------------------------------------------------------------------------
static const PlanetConfig k_sun_config = {
    "Sun",
    0.0f,   // no orbit
    0.0f,
    3.0f,   // self rotation
    3.0f,
    Vec3(1.0f, 0.9f, 0.3f)  // warm yellow
};

static const PlanetConfig k_planet_configs[] = {
    // name       orbit_r  orb_spd  self_rot  radius  color
    { "Mercury",   5.5f,    47.9f,   6.1f,   0.38f,  Vec3(0.7f,  0.65f, 0.6f)  },
    { "Venus",     8.0f,    35.0f,   1.5f,   0.95f,  Vec3(0.9f,  0.75f, 0.5f)  },
    { "Earth",    11.0f,    29.8f,  360.0f,  1.00f,  Vec3(0.2f,  0.5f,  0.9f)  },
    { "Mars",     15.0f,    24.1f,  350.9f,  0.53f,  Vec3(0.8f,  0.35f, 0.2f)  },
    { "Jupiter",  22.0f,    13.1f,  870.0f,  2.50f,  Vec3(0.85f, 0.7f,  0.55f) },
    { "Saturn",   30.0f,     9.7f,  810.0f,  2.10f,  Vec3(0.95f, 0.85f, 0.6f)  },
    { "Uranus",   38.0f,     6.8f,  500.0f,  1.60f,  Vec3(0.55f, 0.85f, 0.95f) },
    { "Neptune",  46.0f,     5.4f,  535.0f,  1.55f,  Vec3(0.2f,  0.4f,  0.95f) },
};

static const int k_planet_count = 8;

static const PlanetConfig k_moon_config = {
    "Moon",
    2.5f,   // orbit radius around Earth
    130.0f, // orbit speed
    130.0f, // self rotation (tidally locked approximation)
    0.27f,
    Vec3(0.8f, 0.8f, 0.8f)
};

// Stacks/slices for sphere geometry.
static const int k_sphere_stacks = 24;
static const int k_sphere_slices = 36;

// Number of segments in an orbit ring.
static const int k_orbit_segments = 128;

// ---------------------------------------------------------------------------
// Helper: extract translation from a 4x4 column-major matrix.
// Column-major layout:  m[12]=tx, m[13]=ty, m[14]=tz.
// ---------------------------------------------------------------------------
static Vec3 extract_translation(const Matrix4& m) {
    return Vec3(m.m[12], m.m[13], m.m[14]);
}

// ---------------------------------------------------------------------------
// Helper: build a shared Material from a color Vec3.
// ---------------------------------------------------------------------------
static std::shared_ptr<Material> make_material(const Vec3& color) {
    auto mat = std::make_shared<Material>();
    mat->ambient = color * 0.15f;
    mat->diffuse = color;
    mat->specular = Vec3(0.4f, 0.4f, 0.4f);
    mat->shininess = 32.0f;
    return mat;
}

// ---------------------------------------------------------------------------
// SolarSystem constructor
// ---------------------------------------------------------------------------
SolarSystem::SolarSystem() {}

// ---------------------------------------------------------------------------
// build()
// ---------------------------------------------------------------------------
void SolarSystem::build(Scene& scene) {
    bodies_.clear();
    orbit_objects_.clear();

    // -- Sun (no parent, no orbit ring) --
    PlanetRecord sun_rec = build_body(scene, k_sun_config, nullptr);
    sun_transform_ = sun_rec.transform;
    bodies_.push_back(std::move(sun_rec));

    // -- Eight planets (parent = sun) --
    for (int i = 0; i < k_planet_count; ++i) {
        PlanetRecord rec = build_body(scene, k_planet_configs[i], sun_transform_);

        if (k_planet_configs[i].name == std::string("Earth")) {
            earth_transform_ = rec.transform;
        }

        rec.orbit_obj = build_orbit(k_planet_configs[i].orbit_radius, sun_transform_);
        bodies_.push_back(std::move(rec));
    }

    // -- Moon (parent = Earth) --
    assert(earth_transform_ != nullptr && "Earth must be built before Moon");
    PlanetRecord moon_rec = build_body(scene, k_moon_config, earth_transform_);
    moon_rec.orbit_obj = build_orbit(k_moon_config.orbit_radius, earth_transform_);
    bodies_.push_back(std::move(moon_rec));
}

// ---------------------------------------------------------------------------
// build_body()
// ---------------------------------------------------------------------------
PlanetRecord SolarSystem::build_body(Scene& scene,
    const PlanetConfig& cfg,
    Transform* parent_transform) {
    // Create geometry.
    Sphere sphere_gen(cfg.radius, k_sphere_stacks, k_sphere_slices);
    auto mesh = sphere_gen.build();  // returns unique_ptr<Mesh>
    mesh->setup_mesh();

    // Create Object and attach geometry + material.
    auto obj = std::make_unique<Object>();
    obj->material = make_material(cfg.color);
    obj->add_mesh(std::move(mesh));

    // Configure transform.
    // The planet starts at angle 0 on the XZ plane: x = orbit_radius, z = 0.
    obj->transform.position = Vec3(cfg.orbit_radius, 0.0f, 0.0f);
    obj->transform.scale = Vec3(1.0f, 1.0f, 1.0f);
    obj->transform.rotation_axis = Vec3(0.0f, 1.0f, 0.0f);
    obj->transform.rotation_angle_deg = 0.0f;
    obj->transform.parent = parent_transform;

    // Keep non-owning pointers before moving ownership into Scene.
    Object* raw_obj = obj.get();
    Transform* raw_transform = &obj->transform;

    scene.add_object(std::move(obj));

    // Build record.
    PlanetRecord rec;
    rec.config = cfg;
    rec.orbit_angle_deg = 0.0f;
    rec.self_rotation_deg = 0.0f;
    rec.object = raw_obj;
    rec.transform = raw_transform;
    rec.orbit_obj = nullptr;

    return rec;
}

Object* SolarSystem::build_orbit(float orbit_radius,
    Transform* parent_transform) {
    Orbit orbit_gen(orbit_radius, k_orbit_segments);
    auto mesh = orbit_gen.build();
    mesh->setup_mesh();

    auto obj = std::make_unique<Object>();
    obj->add_mesh(std::move(mesh));

    obj->transform.position = Vec3(0.0f, 0.0f, 0.0f);
    obj->transform.scale = Vec3(1.0f, 1.0f, 1.0f);
    obj->transform.rotation_axis = Vec3(0.0f, 1.0f, 0.0f);
    obj->transform.rotation_angle_deg = 0.0f;
    obj->transform.parent = parent_transform;

    Object* raw = obj.get();
    orbit_objects_.push_back(std::move(obj));  // SolarSystem 自己持有
    return raw;
}
// ---------------------------------------------------------------------------
// update()
// Advances orbit and self-rotation angles, then recomputes local transforms.
//
// Orbit strategy:
//   We do NOT rotate the parent pivot; instead we directly set the body's
//   local position on a circle of radius orbit_radius in the XZ plane.
//   This keeps Transform::parent clean and avoids double-rotation.
//
// Self-rotation strategy:
//   The local rotation_angle_deg represents spin around Y axis.
// ---------------------------------------------------------------------------
void SolarSystem::update(float delta_time) {
    // Index 0 is the sun -- no orbit, just self-rotation.
    for (PlanetRecord& rec : bodies_) {
        // Advance angles.
        rec.orbit_angle_deg += rec.config.orbit_speed * delta_time;
        rec.self_rotation_deg += rec.config.self_rotation_speed * delta_time;

        // Wrap to [0, 360) to prevent float drift over long sessions.
        if (rec.orbit_angle_deg >= 360.0f) rec.orbit_angle_deg -= 360.0f;
        if (rec.self_rotation_deg >= 360.0f) rec.self_rotation_deg -= 360.0f;

        if (rec.transform == nullptr) continue;

        // Recompute local position on orbit circle (XZ plane, Y = 0).
        float rad = rec.orbit_angle_deg * k_deg_to_rad;
        rec.transform->position = Vec3(
            rec.config.orbit_radius * std::cos(rad),
            0.0f,
            rec.config.orbit_radius * std::sin(rad)
        );

        // Self-rotation around Y axis.
        rec.transform->rotation_axis = Vec3(0.0f, 1.0f, 0.0f);
        rec.transform->rotation_angle_deg = rec.self_rotation_deg;
    }
}

// ---------------------------------------------------------------------------
// submit_orbits()
// ---------------------------------------------------------------------------
void SolarSystem::submit_orbits(Renderer& renderer) const {
    for (const PlanetRecord& rec : bodies_) {
        if (rec.orbit_obj != nullptr) {
            renderer.submit_debug(rec.orbit_obj);
        }
    }
}

// ---------------------------------------------------------------------------
// get_scene_data_for_raytracing()
// ---------------------------------------------------------------------------
std::vector<RenderableSphere> SolarSystem::get_scene_data_for_raytracing() const {
    std::vector<RenderableSphere> result;
    result.reserve(bodies_.size());

    for (const PlanetRecord& rec : bodies_) {
        if (rec.transform == nullptr) continue;

        Matrix4 world = rec.transform->get_world_matrix();

        RenderableSphere rs;
        rs.center = extract_translation(world);
        rs.radius = rec.config.radius;
        rs.color = rec.config.color;
        const std::string& name = rec.config.name;
        if (name == "Sun")          rs.shininess = 0.f;   // 太阳自发光，不反射
        else if (name == "Earth")   rs.shininess = 64.f;  // 海洋反光
        else if (name == "Jupiter") rs.shininess = 48.f;
        else if (name == "Saturn")  rs.shininess = 48.f;
        else                        rs.shininess = 16.f;

        result.push_back(rs);
    }

    return result;
}

void SolarSystem::submit_debug_objects(Renderer& r)
{
    for (const PlanetRecord& rec : bodies_)
    {
        if (rec.orbit_obj != nullptr)
            r.submit_debug(rec.orbit_obj);
    }
}