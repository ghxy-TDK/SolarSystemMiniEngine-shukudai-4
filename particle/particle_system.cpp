#include "../particle/particle_system.h"

#include <cstdlib>   // rand, RAND_MAX
#include <cmath>     // fabsf
#include <GL/glew.h>
#include <iostream>  // debug logging

#include "../render/camera.h"
#include "../render/shader.h"
#include "../math/vec4.h"
#include "../math/matrix4.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static float rand_float_01() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

static float rand_range(float lo, float hi) {
    return lo + rand_float_01() * (hi - lo);
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

ParticleSystem::ParticleSystem(int max_particles, Vec3 emit_pos)
    : max_particles_(max_particles)
    , emit_pos_(emit_pos)
    , vao_(0)
    , vbo_(0)
    , emit_rate_(30.f)       // 30 particles/sec -- tunable
    , spawn_accumulator_(0.f)
{
    // Pre-allocate pool; all particles start dead (life = 0).
    particles_.resize(static_cast<size_t>(max_particles_));
    for (auto& p : particles_) {
        p.life = 0.f;
    }

    init_gl();
    std::cout << "[PS] init_gl vao=" << vao_ << " vbo=" << vbo_ << "\n";
}

ParticleSystem::~ParticleSystem() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
}

// ---------------------------------------------------------------------------
// GL initialisation (called once after glewInit)
// ---------------------------------------------------------------------------

void ParticleSystem::init_gl() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Reserve worst-case buffer: max_particles * sizeof(vec3).
    // Usage hint DYNAMIC_DRAW because we rewrite it every frame.
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(max_particles_) * 3 * sizeof(float),
                 nullptr,
                 GL_DYNAMIC_DRAW);

    // a_position at location 0 -- matches particle.vert
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          reinterpret_cast<void*>(0));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---------------------------------------------------------------------------
// Spawn helper -- fills one dead particle slot with fresh values
// ---------------------------------------------------------------------------

void ParticleSystem::spawn_particle(Particle& p) const {
    p.position = emit_pos_;

    float theta = rand_float_01() * 6.2831853f;
    float phi = rand_float_01() * 3.1415927f;
    float speed = rand_range(5.0f, 10.0f);   // 加大速度确保飞出半径3

    p.velocity.x = speed * std::sin(phi) * std::cos(theta);
    p.velocity.y = speed * std::cos(phi);                    // 修正：cos(phi)
    p.velocity.z = speed * std::sin(phi) * std::sin(theta); // 修正：sin(phi)*sin(theta)

    p.max_life = rand_range(1.5f, 3.0f);
    p.life = p.max_life;
    p.size = rand_range(4.f, 12.f);
    p.color = Vec4(1.0f, rand_range(0.6f, 1.0f), rand_range(0.0f, 0.3f), 1.0f);
}

// ---------------------------------------------------------------------------
// Find the first slot whose life has expired
// ---------------------------------------------------------------------------

int ParticleSystem::find_dead_slot() const {
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        if (particles_[i].life <= 0.f) return i;
    }
    return -1;  // pool full
}

// ---------------------------------------------------------------------------
// Update -- integrate physics, spawn new particles
// ---------------------------------------------------------------------------

void ParticleSystem::update(float dt) {
    // Gravity acceleration (world units/s^2)
    static const Vec3 k_gravity(0.f, -1.8f, 0.f);

    // Integrate live particles
    for (auto& p : particles_) {
        if (p.life <= 0.f) continue;

        p.life     -= dt;
        p.velocity  = p.velocity + k_gravity * dt;
        p.position  = p.position + p.velocity * dt;

        // Alpha fades linearly from 1 to 0 over lifetime
        float fraction  = (p.life > 0.f) ? (p.life / p.max_life) : 0.f;
        p.color.w       = fraction;
    }

    // Spawn new particles to fill dead slots
    spawn_accumulator_ += emit_rate_ * dt;
    while (spawn_accumulator_ >= 1.f) {
        int slot = find_dead_slot();
        if (slot >= 0) {
            spawn_particle(particles_[static_cast<size_t>(slot)]);
        }
        spawn_accumulator_ -= 1.f;
    }
}

// ---------------------------------------------------------------------------
// Upload active particle positions to the GPU
// ---------------------------------------------------------------------------

void ParticleSystem::upload_to_gpu() const {
    // Build a tightly packed float buffer of live particle XYZ positions.
    // We upload positions only; color/size are set as flat uniforms per draw.
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
    std::cout << "[PS] upload " << pos_buf.size() / 3 << " positions to GPU\n";
}

// ---------------------------------------------------------------------------
// Draw -- assumes particle_shader is already bound by the caller
// ---------------------------------------------------------------------------

void ParticleSystem::draw(const Camera& cam, Shader& particle_shader) const {
    // Count live particles first
    int live_count = 0;
    for (const auto& p : particles_) {
        if (p.life > 0.f) ++live_count;
    }

    if (live_count == 0) return;

    // Upload positions to GPU
    upload_to_gpu();

    // Set uniforms required by particle.vert / particle.frag
    // Names are from the 0.4 uniform table.
    particle_shader.set_mat4("u_view",       cam.get_view_matrix());
    std::cout << "[PS] uniforms set\n";
    particle_shader.set_mat4("u_projection", cam.get_projection_matrix());
    std::cout << "[PS] uniforms set\n";

    // Use average colour/size of all live particles for the flat uniforms.
    // A more elaborate system could sort and batch by colour; this is
    // sufficient for a solar-wind / flare effect.
    //
    // For a sun emitter we pick a warm representative value; callers may
    // override by re-binding the shader and setting different uniforms
    // before calling draw().  The flat uniform approach matches the
    // 0.4 table (u_particle_color, u_particle_size).
    particle_shader.set_float("u_particle_size", 50.f);
    std::cout << "[PS] uniforms set\n";

    // Draw each live particle individually so we can set per-particle color.
    // For a small particle count (< 512) this is acceptable; it avoids
    // adding non-table uniforms or VBO color channels.
    // We reuse the same VAO but draw one point at a time by offsetting.
    //
    // Alternative: set a single average color and draw all at once.
    // We choose the simple single-draw approach here to avoid extra VBO
    // layout changes while staying within the uniform table.
    Vec4 avg_color(1.f, 0.8f, 0.2f, 1.f);
    particle_shader.set_vec4("u_particle_color", avg_color);
    std::cout << "[PS] uniforms set\n";

    // Enable blending for additive fire-like effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blend
    glDepthMask(GL_FALSE);              // don't write to depth buffer

    // Enable GL_PROGRAM_POINT_SIZE so the vertex shader controls gl_PointSize
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(vao_);

    // 临时：强制固定管线验证点是否能出现
    glPointSize(20.0f);  // 强制20px，绕过 gl_PointSize
    GLint current_program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    std::cout << "[PS] current GL program=" << current_program
        << " particle_shader.program_id()=" << particle_shader.program_id() << "\n";

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(live_count));
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cout << "[PS] GL error after DrawArrays: 0x" << std::hex << err << "\n";
    glBindVertexArray(0);

    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

// ---------------------------------------------------------------------------
// Emitter position update (called by SolarSystem each frame)
// ---------------------------------------------------------------------------

void ParticleSystem::set_emit_position(Vec3 pos) {
    emit_pos_ = pos;
}
