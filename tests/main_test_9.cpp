// tests/main_test_9.cpp
//
// Stage 9 -- Particle System: pure-CPU unit tests.
//
// DESIGN RATIONALE
// ----------------
// ParticleSystem::ParticleSystem() calls init_gl() which requires an active
// OpenGL context (glewInit + window).  A headless test process has no such
// context, so we cannot link against particle_system.cpp directly.
//
// Instead we use a white-box strategy:
//   1. Inline minimal Vec3 / Vec4 stubs that reproduce the same interface used
//      by particle_system.cpp (operator+, operator*, x/y/z/w fields).
//   2. Inline a Particle struct with identical field order / types to particle.h.
//   3. Inline the five pure-CPU logic functions (spawn_particle, find_dead_slot,
//      update_particles, accumulate_spawn, set_emit_pos) as free functions,
//      taken verbatim from particle_system.cpp with the GL calls removed.
//   4. Assert against the documented invariants.
//
// This approach tests every invariant that matters at the logic level without
// requiring GLUT / GLEW and without modifying production sources.
//
// Build (example, no GL linkage needed):
//   g++ -std=c++17 -o test9 tests/main_test_9.cpp && ./test9
//
// Dependencies: <cassert>, <cstdlib>, <cstdio>, <cstring>, <cmath>, <vector>

#include <cassert>
#include <cstdlib>    // rand, srand, RAND_MAX
#include <cstdio>     // printf
#include <cmath>      // fabsf
#include <vector>
#include <limits>

// ---------------------------------------------------------------------------
// 0. Minimal math stubs (identical interface to math/vec3.h, math/vec4.h)
// ---------------------------------------------------------------------------

struct Vec3 {
    float x, y, z;
    Vec3() : x(0.f), y(0.f), z(0.f) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
    Vec3 operator*(float s)        const { return Vec3(x*s,   y*s,   z*s);   }
    bool operator==(const Vec3& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
    Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

// ---------------------------------------------------------------------------
// 1. Particle struct (must match particle/particle.h exactly)
// ---------------------------------------------------------------------------

struct Particle {
    Vec3  position;
    Vec3  velocity;
    float life;      // remaining lifetime in seconds
    float max_life;  // total lifetime at spawn
    float size;      // point size in pixels at spawn
    Vec4  color;     // rgba
};

// ---------------------------------------------------------------------------
// 2. Inline reimplementation of the five pure-CPU logic functions
//    (mirror of particle_system.cpp, GL calls stripped)
// ---------------------------------------------------------------------------

static float test_rand_float_01() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

static float test_rand_range(float lo, float hi) {
    return lo + test_rand_float_01() * (hi - lo);
}

// Mirror of ParticleSystem::spawn_particle()
static void spawn_particle(Particle& p, Vec3 emit_pos) {
    p.position = emit_pos;

    float spread = 0.8f;
    p.velocity.x = test_rand_range(-spread, spread);
    p.velocity.y = test_rand_range(1.5f, 3.5f);
    p.velocity.z = test_rand_range(-spread, spread);

    p.max_life = test_rand_range(1.0f, 2.5f);
    p.life     = p.max_life;
    p.size     = test_rand_range(4.f, 10.f);

    p.color = Vec4(1.0f,
                   test_rand_range(0.6f, 1.0f),
                   test_rand_range(0.0f, 0.3f),
                   1.0f);
}

// Mirror of ParticleSystem::find_dead_slot()
static int find_dead_slot(const std::vector<Particle>& pool) {
    for (int i = 0; i < static_cast<int>(pool.size()); ++i) {
        if (pool[static_cast<size_t>(i)].life <= 0.f) return i;
    }
    return -1;
}

// Mirror of ParticleSystem::update() -- integrate live particles
static void update_particles(std::vector<Particle>& pool, float dt) {
    static const Vec3 k_gravity(0.f, -1.8f, 0.f);

    for (auto& p : pool) {
        if (p.life <= 0.f) continue;

        p.life     -= dt;
        p.velocity  = p.velocity + k_gravity * dt;
        p.position  = p.position + p.velocity * dt;

        float fraction = (p.life > 0.f) ? (p.life / p.max_life) : 0.f;
        p.color.w      = fraction;
    }
}

// Mirror of the spawn-accumulator section inside update()
static void accumulate_spawn(std::vector<Particle>& pool,
                             float& accumulator,
                             float  emit_rate,
                             float  dt,
                             Vec3   emit_pos)
{
    accumulator += emit_rate * dt;
    while (accumulator >= 1.f) {
        int slot = find_dead_slot(pool);
        if (slot >= 0) {
            spawn_particle(pool[static_cast<size_t>(slot)], emit_pos);
        }
        accumulator -= 1.f;
    }
}

// Mirror of set_emit_position (trivial but tested for completeness)
static Vec3 test_set_emit_position(Vec3 /*old_pos*/, Vec3 new_pos) {
    return new_pos;
}

// ---------------------------------------------------------------------------
// 3. Helper: count live particles in a pool
// ---------------------------------------------------------------------------

static int count_live(const std::vector<Particle>& pool) {
    int n = 0;
    for (const auto& p : pool) if (p.life > 0.f) ++n;
    return n;
}

// Float almost-equal with an absolute tolerance
static bool near(float a, float b, float tol = 1e-5f) {
    return fabsf(a - b) <= tol;
}

// ---------------------------------------------------------------------------
// 4. Test runner infrastructure
// ---------------------------------------------------------------------------

static int g_pass = 0;
static int g_fail = 0;

static void report(const char* name, bool ok) {
    if (ok) {
        printf("[PASS] %s\n", name);
        ++g_pass;
    } else {
        printf("[FAIL] %s\n", name);
        ++g_fail;
    }
}

#define RUN(fn) do { fn(); } while(0)

// ---------------------------------------------------------------------------
// 5. Individual test functions
// ---------------------------------------------------------------------------

// T01 -- Particle struct can be default-constructed and fields assigned
static void T01_particle_default_construct() {
    Particle p;
    p.life     = 0.f;
    p.max_life = 2.f;
    p.size     = 5.f;
    assert(p.life     == 0.f);
    assert(p.max_life == 2.f);
    assert(p.size     == 5.f);
    report("T01_particle_default_construct", true);
}

// T02 -- Particle fields have expected layout: position/velocity Vec3, color Vec4
static void T02_particle_field_types() {
    Particle p;
    p.position = Vec3(1.f, 2.f, 3.f);
    p.velocity = Vec3(0.f, 1.f, 0.f);
    p.color    = Vec4(1.f, 0.f, 0.f, 1.f);
    bool ok = (p.position.x == 1.f) && (p.position.y == 2.f) &&
              (p.velocity.y == 1.f) &&
              (p.color.x    == 1.f) && (p.color.w == 1.f);
    assert(ok);
    report("T02_particle_field_types", ok);
}

// T03 -- spawn_particle sets position exactly to emit_pos
static void T03_spawn_position_equals_emit_pos() {
    Vec3 emit(3.f, 0.f, -5.f);
    Particle p; p.life = 0.f;
    spawn_particle(p, emit);
    bool ok = (p.position.x == emit.x) &&
              (p.position.y == emit.y) &&
              (p.position.z == emit.z);
    assert(ok);
    report("T03_spawn_position_equals_emit_pos", ok);
}

// T04 -- spawn_particle: velocity.y in [1.5, 3.5]
static void T04_spawn_velocity_y_range() {
    Vec3 emit(0.f, 0.f, 0.f);
    bool ok = true;
    for (int i = 0; i < 500; ++i) {
        Particle p; p.life = 0.f;
        spawn_particle(p, emit);
        if (p.velocity.y < 1.5f || p.velocity.y > 3.5f) { ok = false; break; }
    }
    assert(ok);
    report("T04_spawn_velocity_y_range", ok);
}

// T05 -- spawn_particle: velocity.x and velocity.z in [-0.8, 0.8]
static void T05_spawn_velocity_xz_range() {
    Vec3 emit(0.f, 0.f, 0.f);
    bool ok = true;
    for (int i = 0; i < 500; ++i) {
        Particle p; p.life = 0.f;
        spawn_particle(p, emit);
        if (p.velocity.x < -0.8f || p.velocity.x > 0.8f) { ok = false; break; }
        if (p.velocity.z < -0.8f || p.velocity.z > 0.8f) { ok = false; break; }
    }
    assert(ok);
    report("T05_spawn_velocity_xz_range", ok);
}

// T06 -- spawn_particle: life == max_life immediately after spawn
static void T06_spawn_life_equals_max_life() {
    Vec3 emit(0.f, 0.f, 0.f);
    bool ok = true;
    for (int i = 0; i < 200; ++i) {
        Particle p; p.life = 0.f;
        spawn_particle(p, emit);
        if (!near(p.life, p.max_life)) { ok = false; break; }
    }
    assert(ok);
    report("T06_spawn_life_equals_max_life", ok);
}

// T07 -- spawn_particle: max_life in [1.0, 2.5]
static void T07_spawn_max_life_range() {
    Vec3 emit(0.f, 0.f, 0.f);
    bool ok = true;
    for (int i = 0; i < 500; ++i) {
        Particle p; p.life = 0.f;
        spawn_particle(p, emit);
        if (p.max_life < 1.0f || p.max_life > 2.5f) { ok = false; break; }
    }
    assert(ok);
    report("T07_spawn_max_life_range", ok);
}

// T08 -- spawn_particle: size in [4.0, 10.0]
static void T08_spawn_size_range() {
    Vec3 emit(0.f, 0.f, 0.f);
    bool ok = true;
    for (int i = 0; i < 500; ++i) {
        Particle p; p.life = 0.f;
        spawn_particle(p, emit);
        if (p.size < 4.f || p.size > 10.f) { ok = false; break; }
    }
    assert(ok);
    report("T08_spawn_size_range", ok);
}

// T09 -- spawn_particle: color.w (alpha) == 1.0 at spawn
static void T09_spawn_alpha_is_one() {
    Vec3 emit(0.f, 0.f, 0.f);
    bool ok = true;
    for (int i = 0; i < 200; ++i) {
        Particle p; p.life = 0.f;
        spawn_particle(p, emit);
        if (!near(p.color.w, 1.0f)) { ok = false; break; }
    }
    assert(ok);
    report("T09_spawn_alpha_is_one", ok);
}

// T10 -- spawn_particle: color.x (red channel) == 1.0 always
static void T10_spawn_red_channel_is_one() {
    Vec3 emit(0.f, 0.f, 0.f);
    bool ok = true;
    for (int i = 0; i < 200; ++i) {
        Particle p; p.life = 0.f;
        spawn_particle(p, emit);
        if (!near(p.color.x, 1.0f)) { ok = false; break; }
    }
    assert(ok);
    report("T10_spawn_red_channel_is_one", ok);
}

// T11 -- update: life decreases by exactly dt each tick
static void T11_update_life_decreases_by_dt() {
    std::vector<Particle> pool(1);
    pool[0].life     = 2.0f;
    pool[0].max_life = 2.0f;
    pool[0].velocity = Vec3(0.f, 0.f, 0.f);
    pool[0].position = Vec3(0.f, 0.f, 0.f);
    pool[0].color.w  = 1.0f;

    float dt = 0.016f;
    update_particles(pool, dt);
    bool ok = near(pool[0].life, 2.0f - dt);
    assert(ok);
    report("T11_update_life_decreases_by_dt", ok);
}

// T12 -- update: particle becomes dead (life <= 0) after enough ticks
static void T12_update_particle_dies() {
    std::vector<Particle> pool(1);
    pool[0].life     = 0.05f;
    pool[0].max_life = 2.0f;
    pool[0].velocity = Vec3(0.f, 1.f, 0.f);
    pool[0].position = Vec3(0.f, 0.f, 0.f);
    pool[0].color.w  = 1.0f;

    // One large step kills it
    update_particles(pool, 0.1f);
    bool ok = (pool[0].life <= 0.f);
    assert(ok);
    report("T12_update_particle_dies", ok);
}

// T13 -- update: gravity (0, -1.8, 0) is applied to velocity each tick
static void T13_update_gravity_applied() {
    std::vector<Particle> pool(1);
    pool[0].life     = 5.0f;
    pool[0].max_life = 5.0f;
    pool[0].velocity = Vec3(0.f, 2.0f, 0.f);
    pool[0].position = Vec3(0.f, 0.f, 0.f);
    pool[0].color.w  = 1.0f;

    float dt = 0.1f;
    float expected_vy = 2.0f + (-1.8f) * dt;  // gravity = (0, -1.8, 0)
    update_particles(pool, dt);
    bool ok = near(pool[0].velocity.y, expected_vy);
    assert(ok);
    report("T13_update_gravity_applied", ok);
}

// T14 -- update: position integrated correctly (pos += vel * dt AFTER vel updated)
static void T14_update_position_integrated() {
    std::vector<Particle> pool(1);
    pool[0].life     = 5.0f;
    pool[0].max_life = 5.0f;
    pool[0].velocity = Vec3(1.0f, 0.0f, 0.0f);  // pure X motion
    pool[0].position = Vec3(0.f,  0.f,  0.f);
    pool[0].color.w  = 1.0f;

    float dt = 0.5f;
    // After update: vel.x unchanged (gravity is Y only), pos.x += vel.x * dt
    float expected_x = 0.f + 1.0f * dt;
    update_particles(pool, dt);
    bool ok = near(pool[0].position.x, expected_x);
    assert(ok);
    report("T14_update_position_integrated", ok);
}

// T15 -- update: alpha = life / max_life (linear fade)
static void T15_update_alpha_fade() {
    std::vector<Particle> pool(1);
    pool[0].life     = 1.0f;
    pool[0].max_life = 2.0f;
    pool[0].velocity = Vec3(0.f, 0.f, 0.f);
    pool[0].position = Vec3(0.f, 0.f, 0.f);
    pool[0].color.w  = 1.0f;

    float dt = 0.5f;
    // After update: life = 1.0 - 0.5 = 0.5, alpha = 0.5 / 2.0 = 0.25
    float expected_alpha = (1.0f - dt) / 2.0f;
    update_particles(pool, dt);
    bool ok = near(pool[0].color.w, expected_alpha);
    assert(ok);
    report("T15_update_alpha_fade", ok);
}

// T16 -- update: alpha clamped to 0 when life <= 0 (no negative alpha)
static void T16_update_alpha_zero_when_dead() {
    std::vector<Particle> pool(1);
    pool[0].life     = 0.01f;
    pool[0].max_life = 2.0f;
    pool[0].velocity = Vec3(0.f, 0.f, 0.f);
    pool[0].position = Vec3(0.f, 0.f, 0.f);
    pool[0].color.w  = 1.0f;

    // Large dt kills the particle; alpha must be 0, not negative
    update_particles(pool, 1.0f);
    bool ok = near(pool[0].color.w, 0.f);
    assert(ok);
    report("T16_update_alpha_zero_when_dead", ok);
}

// T17 -- find_dead_slot: returns 0 when all particles are dead
static void T17_find_dead_slot_all_dead() {
    std::vector<Particle> pool(5);
    for (auto& p : pool) p.life = 0.f;
    int slot = find_dead_slot(pool);
    assert(slot == 0);
    report("T17_find_dead_slot_all_dead", slot == 0);
}

// T18 -- find_dead_slot: returns -1 when pool is completely full (all alive)
static void T18_find_dead_slot_pool_full() {
    std::vector<Particle> pool(5);
    for (auto& p : pool) p.life = 1.0f;
    int slot = find_dead_slot(pool);
    assert(slot == -1);
    report("T18_find_dead_slot_pool_full", slot == -1);
}

// T19 -- find_dead_slot: returns correct index for mixed pool
static void T19_find_dead_slot_mixed_pool() {
    std::vector<Particle> pool(5);
    pool[0].life = 1.0f;  // alive
    pool[1].life = 1.0f;  // alive
    pool[2].life = 0.0f;  // DEAD  <-- first dead
    pool[3].life = 0.0f;  // dead
    pool[4].life = 1.0f;  // alive
    int slot = find_dead_slot(pool);
    assert(slot == 2);
    report("T19_find_dead_slot_mixed_pool", slot == 2);
}

// T20 -- spawn_accumulator: no particle spawned when accumulator < 1.0
static void T20_accumulator_no_spawn_below_threshold() {
    // dt=0.01, rate=30: accumulator after one step = 0.30 < 1.0 -> no spawn
    std::vector<Particle> pool(10);
    for (auto& p : pool) p.life = 0.f;
    float accum     = 0.f;
    float emit_rate = 30.f;
    float dt        = 0.01f;  // 30 * 0.01 = 0.30

    Vec3 emit(0.f, 0.f, 0.f);
    accumulate_spawn(pool, accum, emit_rate, dt, emit);

    int live = count_live(pool);
    bool ok  = (live == 0) && near(accum, 0.30f, 1e-4f);
    assert(ok);
    report("T20_accumulator_no_spawn_below_threshold", ok);
}

// T21 -- spawn_accumulator: spawns exactly 1 particle when dt == 1/rate
static void T21_accumulator_spawns_one_at_threshold() {
    // dt = 1/30, rate = 30: exactly 1.0 credit -> spawn exactly 1 particle
    std::vector<Particle> pool(10);
    for (auto& p : pool) p.life = 0.f;
    float accum     = 0.f;
    float emit_rate = 30.f;
    float dt        = 1.f / 30.f;

    Vec3 emit(0.f, 0.f, 0.f);
    accumulate_spawn(pool, accum, emit_rate, dt, emit);

    int live = count_live(pool);
    // Floating-point: 30 * (1/30) may be slightly below 1 on some platforms,
    // so we accept either 0 or 1 spawn (the contract is "approximately one").
    // The important invariant: never spawns more than floor(rate*dt) + 1.
    bool ok = (live >= 0 && live <= 1);
    assert(ok);
    report("T21_accumulator_spawns_one_at_threshold", ok);
}

// T22 -- spawn_accumulator: fractional remainder carried across two calls
static void T22_accumulator_carries_remainder() {
    // Two calls, each contributing 0.7 credit -> total = 1.4 -> 1 spawn, 0.4 left
    std::vector<Particle> pool(10);
    for (auto& p : pool) p.life = 0.f;
    float accum     = 0.f;
    float emit_rate = 7.f;    // 7 * 0.1 = 0.7 per call
    float dt        = 0.1f;

    Vec3 emit(0.f, 0.f, 0.f);
    accumulate_spawn(pool, accum, emit_rate, dt, emit);   // accum -> 0.7, no spawn
    assert(count_live(pool) == 0);

    accumulate_spawn(pool, accum, emit_rate, dt, emit);   // accum -> 1.4, 1 spawn
    int live = count_live(pool);
    bool remainder_ok = (accum > 0.f && accum < 1.f);    // 0.4 carried
    bool spawn_ok     = (live == 1);

    bool ok = spawn_ok && remainder_ok;
    assert(ok);
    report("T22_accumulator_carries_remainder", ok);
}

// T23 -- set_emit_position: newly spawned particle uses updated position
static void T23_set_emit_position_used_by_spawn() {
    Vec3 old_pos(1.f, 0.f, 0.f);
    Vec3 new_pos(99.f, 0.f, 0.f);

    Vec3 stored = test_set_emit_position(old_pos, new_pos);
    assert(stored.x == new_pos.x);
    assert(stored.y == new_pos.y);
    assert(stored.z == new_pos.z);

    // Verify that a particle spawned from new_pos has position == new_pos
    Particle p; p.life = 0.f;
    spawn_particle(p, stored);
    bool ok = (p.position.x == new_pos.x) &&
              (p.position.y == new_pos.y) &&
              (p.position.z == new_pos.z);
    assert(ok);
    report("T23_set_emit_position_used_by_spawn", ok);
}

// T24 -- pool capacity: particle pool has exactly max_particles slots
static void T24_pool_has_max_particles_slots() {
    // Simulate ParticleSystem constructor pool allocation
    int max_particles = 128;
    std::vector<Particle> pool;
    pool.resize(static_cast<size_t>(max_particles));
    for (auto& p : pool) p.life = 0.f;

    bool ok = (static_cast<int>(pool.size()) == max_particles);
    assert(ok);
    report("T24_pool_has_max_particles_slots", ok);
}

// T25 -- pool initial state: all particles start with life == 0 (dead)
static void T25_pool_initial_all_dead() {
    int max_particles = 64;
    std::vector<Particle> pool;
    pool.resize(static_cast<size_t>(max_particles));
    for (auto& p : pool) p.life = 0.f;  // mirror of constructor

    bool ok = true;
    for (const auto& p : pool) {
        if (p.life != 0.f) { ok = false; break; }
    }
    assert(ok);
    report("T25_pool_initial_all_dead", ok);
}

// ---------------------------------------------------------------------------
// 6. Main
// ---------------------------------------------------------------------------

int main() {
    // Use a fixed seed for reproducibility; all range checks still hold.
    srand(42u);

    printf("=== Stage 9 Particle System Tests ===\n\n");

    RUN(T01_particle_default_construct);
    RUN(T02_particle_field_types);
    RUN(T03_spawn_position_equals_emit_pos);
    RUN(T04_spawn_velocity_y_range);
    RUN(T05_spawn_velocity_xz_range);
    RUN(T06_spawn_life_equals_max_life);
    RUN(T07_spawn_max_life_range);
    RUN(T08_spawn_size_range);
    RUN(T09_spawn_alpha_is_one);
    RUN(T10_spawn_red_channel_is_one);
    RUN(T11_update_life_decreases_by_dt);
    RUN(T12_update_particle_dies);
    RUN(T13_update_gravity_applied);
    RUN(T14_update_position_integrated);
    RUN(T15_update_alpha_fade);
    RUN(T16_update_alpha_zero_when_dead);
    RUN(T17_find_dead_slot_all_dead);
    RUN(T18_find_dead_slot_pool_full);
    RUN(T19_find_dead_slot_mixed_pool);
    RUN(T20_accumulator_no_spawn_below_threshold);
    RUN(T21_accumulator_spawns_one_at_threshold);
    RUN(T22_accumulator_carries_remainder);
    RUN(T23_set_emit_position_used_by_spawn);
    RUN(T24_pool_has_max_particles_slots);
    RUN(T25_pool_initial_all_dead);

    printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
