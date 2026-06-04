#include "scene.h"

// ---------------------------------------------------------------------------
// add
// ---------------------------------------------------------------------------
void Scene::add_object(std::unique_ptr<Object> obj) {
    if (obj) objects_.push_back(std::move(obj));
}

void Scene::add_light(std::unique_ptr<PointLight> light) {
    if (light) lights_.push_back(std::move(light));
}

void Scene::add_particle_system(std::unique_ptr<ParticleSystem> ps) {
    if (ps) particle_systems_.push_back(std::move(ps));
}

// ---------------------------------------------------------------------------
// non-owning views
// ---------------------------------------------------------------------------
std::vector<Object*> Scene::get_objects() const {
    std::vector<Object*> out;
    out.reserve(objects_.size());
    for (const auto& p : objects_) out.push_back(p.get());
    return out;
}

std::vector<PointLight*> Scene::get_lights() const {
    std::vector<PointLight*> out;
    out.reserve(lights_.size());
    for (const auto& p : lights_) out.push_back(p.get());
    return out;
}

std::vector<ParticleSystem*> Scene::get_particle_systems() const {
    std::vector<ParticleSystem*> out;
    out.reserve(particle_systems_.size());
    for (const auto& p : particle_systems_) out.push_back(p.get());
    return out;
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------
void Scene::update(float delta_time) {
    for (const auto& ps : particle_systems_) {
        if (ps) ps->update(delta_time);
    }
}
