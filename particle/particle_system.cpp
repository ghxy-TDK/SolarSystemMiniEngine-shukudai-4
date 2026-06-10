#include "../particle/particle_system.h"

#include <cstdlib>
#include <cmath>
#include <GL/glew.h>
#include <iostream>

#include "../render/camera.h"
#include "../render/shader.h"
#include "../math/vec4.h"
#include "../math/matrix4.h"

static float rand_float_01() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

static float rand_range(float lo, float hi) {
    return lo + rand_float_01() * (hi - lo);
}

ParticleSystem::ParticleSystem(int max_particles, Vec3 emit_pos)
    : max_particles_(max_particles)
    , emit_pos_(emit_pos)
    , vao_(0)
    , vbo_(0)
    , emit_rate_(100.f)
    , spawn_accumulator_(0.f)
{
    particles_.resize(static_cast<size_t>(max_particles_));
    for (auto& p : particles_) {
        p.life = 0.f;
    }
    init_gl();
}

ParticleSystem::~ParticleSystem() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
}

void ParticleSystem::init_gl() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(max_particles_) * 3 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
        reinterpret_cast<void*>(0));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::spawn_particle(Particle& p) const {
    p.position = emit_pos_;

    float theta = rand_float_01() * 6.2831853f;
    float phi = rand_float_01() * 3.1415927f;
    float speed = rand_range(20.0f, 50.0f);

    p.velocity.x = speed * std::sin(phi) * std::cos(theta);
    p.velocity.y = speed * std::cos(phi);
    p.velocity.z = speed * std::sin(phi) * std::sin(theta);

    p.max_life = rand_range(8.0f, 15.0f);
    p.life = p.max_life;
    p.size = rand_range(4.f, 12.f);
    p.color = Vec4(1.0f, rand_range(0.6f, 1.0f), rand_range(0.0f, 0.3f), 1.0f);
}

int ParticleSystem::find_dead_slot() const {
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        if (particles_[i].life <= 0.f) return i;
    }
    return -1;
}

void ParticleSystem::update(float dt) {
    static const Vec3 k_gravity(0.f, 0.f, 0.f);

    for (auto& p : particles_) {
        if (p.life <= 0.f) continue;
        p.life -= dt;
        p.velocity = p.velocity + k_gravity * dt;
        p.position = p.position + p.velocity * dt;
        float fraction = (p.life > 0.f) ? (p.life / p.max_life) : 0.f;
        p.color.w = fraction;
    }

    spawn_accumulator_ += emit_rate_ * dt;
    while (spawn_accumulator_ >= 1.f) {
        int slot = find_dead_slot();
        if (slot >= 0)
            spawn_particle(particles_[static_cast<size_t>(slot)]);
        spawn_accumulator_ -= 1.f;
    }
}

void ParticleSystem::upload_to_gpu() const {
    static std::vector<float> pos_buf;
    pos_buf.clear();
    pos_buf.reserve(static_cast<size_t>(max_particles_) * 3);

    for (const auto& p : particles_) {
        if (p.life <= 0.f) continue;
        pos_buf.push_back(p.position.x);
        pos_buf.push_back(p.position.y);
        pos_buf.push_back(p.position.z);
    }

    if (pos_buf.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(pos_buf.size()) * sizeof(float),
        pos_buf.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::draw(const Camera& cam, Shader& particle_shader) const {
    int live_count = 0;
    for (const auto& p : particles_) {
        if (p.life > 0.f) ++live_count;
    }
    if (live_count == 0) return;

    upload_to_gpu();

    particle_shader.set_mat4("u_view", cam. get_view_matrix());
    particle_shader.set_mat4("u_projection", cam.get_projection_matrix());
    particle_shader.set_float("u_particle_size", 5.f);

    Vec4 avg_color(1.f, 0.8f, 0.2f, 1.f);
    particle_shader.set_vec4("u_particle_color", avg_color);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(vao_);

    GLfloat point_size = 0.f;
    glGetFloatv(GL_POINT_SIZE, &point_size);
    GLboolean ps_on = glIsEnabled(GL_PROGRAM_POINT_SIZE);
    std::cout << "[PS] before draw: point_size=" << point_size
        << " PROGRAM_POINT_SIZE=" << (int)ps_on
        << " live=" << live_count << "\n";

    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(live_count));
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    //glDisable(GL_PROGRAM_POINT_SIZE);
}

void ParticleSystem::set_emit_position(Vec3 pos) {
    emit_pos_ = pos;
}