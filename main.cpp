// main.cpp  --  Solar System Mini Engine  Stage 11: Final Integration
// ASCII-only source. No Unicode, no em-dashes, no box-drawing characters.

#include <iostream>
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>

// GLEW before GLUT
#include <GL/glew.h>
#include <GL/freeglut.h>

// Engine headers
#include "core/application.h"
// NOTE: core/timer.h is intentionally NOT included.
// Timer is a namespace in that header, not a class.
// Delta-time is computed manually via glutGet(GLUT_ELAPSED_TIME).

#include "math/vec3.h"
#include "math/matrix4.h"

#include "render/camera.h"
#include "render/shader.h"
#include "render/renderer.h"
#include "render/material.h"

#include "model/model.h"
#include "scene/scene.h"
#include "scene/object.h"
#include "scene/solar_system.h"   // defines RenderableSphere

#include "particle/particle_system.h"

// whitted.h also defines RenderableSphere as a local copy.
// To avoid the C2011 redefinition error, solar_system.h must be included
// BEFORE whitted.h so the compiler sees the first definition, and whitted.h
// must use an include guard / pragma once (it does: #pragma once).
// Because both structs have identical layout the ODR is satisfied at link
// time; at compile time the second definition in whitted.h is silently
// skipped by #pragma once when both are in the same translation unit only
// if they come through different headers -- they do NOT in this TU, so we
// rely on the linker-level ODR. The safe permanent fix is to remove the
// duplicate from whitted.h and have it #include "scene/solar_system.h",
// but that requires touching whitted.h. As a zero-touch-to-other-files
// workaround we define a guard macro before including whitted.h:
#define RENDERABLE_SPHERE_DEFINED
#include "raytracing/whitted.h"
// (whitted.h must wrap its RenderableSphere block with:
//    #ifndef RENDERABLE_SPHERE_DEFINED
//    struct RenderableSphere { ... };
//    #endif
// If you cannot modify whitted.h, see the alternative at the bottom of
// this file.)

// ---------------------------------------------------------------------------
// Ray-trace pending flag (set by keyboard callback, read by display callback)
// ---------------------------------------------------------------------------
static bool g_raytrace_pending = false;

// ---------------------------------------------------------------------------
// Non-owning observer pointers (all objects are owned by main() stack vars)
// ---------------------------------------------------------------------------
static Camera* g_camera = nullptr;
static Scene* g_scene = nullptr;
static SolarSystem* g_solar_system = nullptr;
static Renderer* g_renderer = nullptr;
static Shader* g_phong_shader = nullptr;
static Shader* g_particle_shader = nullptr;
static Shader* g_line_shader = nullptr;
static bool g_model_loaded = false;

// ---------------------------------------------------------------------------
// Delta-time via GLUT elapsed time (avoids the Timer-namespace conflict)
// ---------------------------------------------------------------------------
static int g_last_time_ms = 0;

static float compute_delta_time()
{
    int   now = glutGet(GLUT_ELAPSED_TIME);
    float dt = static_cast<float>(now - g_last_time_ms) * 0.001f;
    g_last_time_ms = now;
    if (dt > 0.1f) dt = 0.1f; // guard against stalls
    if (dt < 0.001f) dt = 0.016f;
    return dt;
}

// Mirrors the initial Camera position; kept in sync so Whitted can use it.
// If your Camera later exposes get_position(), replace g_cam_pos with that.
static Vec3  g_cam_pos = { 0.f, 10.f, 40.f };
static Vec3  g_cam_lookat = { 0.f,  0.f,  0.f };  // look-at target for Whitted
static float k_cam_fov_deg = 45.f;

// Mouse free-look
static int  g_mouse_last_x = -1;
static int  g_mouse_last_y = -1;
static bool g_mouse_captured = false;

// ---------------------------------------------------------------------------
// Feature toggles
// ---------------------------------------------------------------------------
static int  g_lighting_model = 2; // 0=Ambient 1=Lambert 2=Phong 3=Blinn
static bool g_particles_on = false;
static bool g_orbits_on = true;

// Window dimensions (updated in reshape; used by Whitted params)
static int g_win_w = 800;
static int g_win_h = 600;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void display_callback();
static void reshape_callback(int w, int h);
static void keyboard_down_callback(unsigned char key, int x, int y);
static void keyboard_up_callback(unsigned char key, int x, int y);
static void mouse_callback(int button, int state, int x, int y);
static void motion_callback(int x, int y);
static void idle_callback();

static void display_callback()
{
    // --- Whitted ray-trace path (Section 0.6 freeze-frame protocol) ---
    if (g_raytrace_pending)
    {
        std::cout << "[Raytrace] Freezing display. Starting Whitted render...\n";
        std::cout << "[Raytrace] Window unresponsive until render completes.\n";

        std::filesystem::create_directories("screenshots");

        std::vector<RenderableSphere> spheres =
            g_solar_system->get_scene_data_for_raytracing();

        Whitted::Params p;
        p.width = g_win_w;
        p.height = g_win_h;
        p.max_depth = 4;
        p.eye = g_camera->position();
        p.lookat = {
            g_camera->position().x + g_camera->front().x,
            g_camera->position().y + g_camera->front().y,
            g_camera->position().z + g_camera->front().z
        };
        p.up = Vec3{ 0.f, 1.f, 0.f };
        p.fov_deg = k_cam_fov_deg;

        p.light.position = Vec3{ 0.f, 0.f, 0.f };
        p.light.color = Vec3{ 1.f, 0.95f, 0.8f };
        p.light.intensity = 800.f;

        p.ambient_intensity = 0.05f;

        p.reflection_threshold = 16.f;
        p.reflection_strength = 0.25f;

        Whitted::render(spheres, p, "screenshots/raytrace.bmp");

        std::cout << "[Raytrace] Done. Saved to screenshots/raytrace.bmp\n";

        g_raytrace_pending = false;
        glutPostRedisplay();
        return;
    }

    // --- Normal render frame ---
    float dt = compute_delta_time();

    g_camera->process_keyboard(dt);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_solar_system->update(dt);
    g_scene->update(dt);

    // Build render queues for this frame
    g_renderer->clear_queue();

    for (auto* obj : g_scene->get_objects())
        g_renderer->submit_opaque(obj);

    if (g_particles_on)
        for (auto* ps : g_scene->get_particle_systems())
            g_renderer->submit_transparent(ps);

    if (g_orbits_on)
        g_solar_system->submit_debug_objects(*g_renderer);

    // Per-frame uniforms: lighting model + point light
    g_phong_shader->use();
    g_phong_shader->set_int("u_lighting_model", g_lighting_model);
    g_phong_shader->set_vec3("u_point_light.position", Vec3{ 0.f, 0.f, 0.f });
    g_phong_shader->set_vec3("u_point_light.color", Vec3{ 1.f, 1.f, 0.9f });
    g_phong_shader->set_float("u_point_light.intensity", 2.0f);

    // Three render passes
    g_renderer->render_opaque(*g_camera, *g_phong_shader);
    g_renderer->render_transparent(*g_camera, *g_particle_shader);

    g_renderer->render_debug(*g_camera, *g_line_shader);

    glutSwapBuffers();
}

// ---------------------------------------------------------------------------
// idle_callback
// ---------------------------------------------------------------------------
static void idle_callback()
{
    // Do not post redraws while a ray-trace is pending; display_callback
    // will re-post itself after the render completes.
    if (!g_raytrace_pending)
        glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// reshape_callback
// ---------------------------------------------------------------------------
static void reshape_callback(int w, int h)
{
    if (h == 0) h = 1;
    g_win_w = w;
    g_win_h = h;
    glViewport(0, 0, w, h);
    // Camera snapshot has no set_aspect(); Camera manages its own aspect.
}

// ---------------------------------------------------------------------------
// keyboard_down_callback
// ---------------------------------------------------------------------------
static void keyboard_down_callback(unsigned char key, int /*x*/, int /*y*/)
{
    // 转发给 Camera
    if (g_camera)
        g_camera->set_key(key, true);
    
    switch (key)
    {
    case 27: glutLeaveMainLoop(); break; // ESC

        // Lighting model (u_lighting_model values per Section 0.4)
    case '1': g_lighting_model = 0; break; // Ambient
    case '2': g_lighting_model = 1; break; // Lambert
    case '3': g_lighting_model = 2; break; // Phong
    case '4': g_lighting_model = 3; break; // Blinn-Phong

    case 'p': case 'P':
        g_particles_on = !g_particles_on;
        std::cout << "[Toggle] Particles: " << (g_particles_on ? "ON" : "OFF") << "\n";
        break;

    case 'o': case 'O':
        g_orbits_on = !g_orbits_on;
        std::cout << "[Toggle] Orbit lines: " << (g_orbits_on ? "ON" : "OFF") << "\n";
        break;

    case 'r': case 'R':
        if (!g_raytrace_pending)
        {
            g_raytrace_pending = true;
            std::cout << "[Raytrace] R pressed -- ray-trace queued.\n";
        }
        break;

    case 'm': case 'M':
        if (!g_model_loaded) {
            namespace fs = std::filesystem;
            fs::path obj_path = "assets/models/test.obj";
            if (fs::exists(obj_path)) {
                Model model_obj(obj_path);
                model_obj.setup_all_meshes();

                auto obj = std::make_unique<Object>("imported_model");
                obj->transform.position = Vec3{ 15.f, 0.f, 0.f };
                obj->transform.scale = Vec3{ 0.5f, 0.5f, 0.5f };
                obj->material = std::make_shared<Material>();
                obj->material->diffuse = Vec3{ 0.8f, 0.6f, 0.4f };
                obj->material->specular = Vec3{ 0.3f, 0.3f, 0.3f };
                obj->material->shininess = 16.f;

                model_obj.inject_into(*obj);
                g_scene->add_object(std::move(obj));
                g_model_loaded = true;
                std::cout << "[Model] Loaded: " << obj_path << "\n";
            }
            else {
                std::cout << "[Model] File not found: " << obj_path << "\n";
            }
        }
        else {
            std::cout << "[Model] Already loaded.\n";
        }
        break;

    default: break;
    }
}

// ---------------------------------------------------------------------------
// keyboard_up_callback
// ---------------------------------------------------------------------------
static void keyboard_up_callback(unsigned char key, int /*x*/, int /*y*/)
{
    if (g_camera)
        g_camera->set_key(key, false);
    
    switch (key)
    {
    default: break;
    }
}

// ---------------------------------------------------------------------------
// mouse_callback -- right-click to enter / leave free-look
// ---------------------------------------------------------------------------
static void mouse_callback(int button, int state, int x, int y)
{
    if (button == GLUT_RIGHT_BUTTON)
    {
        g_mouse_captured = (state == GLUT_DOWN);
        g_mouse_last_x = x;
        g_mouse_last_y = y;
    }
}

// ---------------------------------------------------------------------------
// motion_callback -- free-look while right mouse button held
// ---------------------------------------------------------------------------
static void motion_callback(int x, int y)
{
    if (!g_mouse_captured) return;

    float dx = static_cast<float>(x - g_mouse_last_x);
    float dy = static_cast<float>(y - g_mouse_last_y);
    g_mouse_last_x = x;
    g_mouse_last_y = y;

    g_camera->process_mouse(dx, dy);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // Step 1: Application::init -- GLUT + GLEW + GL debug callback
    Application& app = Application::instance();
    if (!app.init(argc, argv))
    {
        std::cerr << "[main] Application::init() failed.\n";
        return 1;
    }

    // Step 2: Camera -- pure math, no GL
    Camera camera(g_cam_pos, -90.f, -10.f);
    g_camera = &camera;

    // Step 3: Shaders -- require GL context
    Shader phong_shader("assets/shaders/phong.vert", "assets/shaders/phong.frag");
    Shader particle_shader("assets/shaders/particle.vert", "assets/shaders/particle.frag");
    Shader line_shader("assets/shaders/line.vert", "assets/shaders/line.frag");
    g_phong_shader = &phong_shader;
    g_particle_shader = &particle_shader;
    g_line_shader = &line_shader;

    // Step 4: Scene -- no GL
    Scene scene;
    g_scene = &scene;

    // Step 5: SolarSystem -- builds Object/Mesh, requires GL
    SolarSystem solar_system;
    solar_system.build(scene);
    g_solar_system = &solar_system;

    // Step 6: ParticleSystem -- requires GL; owned by scene
    // Adjust constructor arguments to match your actual ParticleSystem API.
    auto corona_ps = std::make_unique<ParticleSystem>(
        2000,                 // max particles
        Vec3{ 0.f, 0.f, 0.f }// emitter at sun center
    );
    scene.add_particle_system(std::move(corona_ps));

    // Step 7: Renderer -- no GL
    Renderer renderer;
    g_renderer = &renderer;

    // Initial GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Seed delta-time baseline
    g_last_time_ms = glutGet(GLUT_ELAPSED_TIME);

    // Register GLUT callbacks
    glutDisplayFunc(display_callback);
    glutReshapeFunc(reshape_callback);
    glutKeyboardFunc(keyboard_down_callback);
    glutKeyboardUpFunc(keyboard_up_callback);
    glutMouseFunc(mouse_callback);
    glutMotionFunc(motion_callback);
    glutIdleFunc(idle_callback);

    std::cout << "=== Solar System Mini Engine ===\n";
    std::cout << "  ESC     : Quit\n";
    std::cout << "  1/2/3/4 : Lighting (Ambient/Lambert/Phong/Blinn)\n";
    std::cout << "  WASD    : Camera move\n";
    std::cout << "  RMB     : Hold to look around\n";
    std::cout << "  P       : Toggle particles\n";
    std::cout << "  O       : Toggle orbit lines\n";
    std::cout << "  R       : Ray-trace -> screenshots/raytrace.bmp\n";
    std::cout << "  M       : Import OBJ model\n";
    std::cout << "================================\n";

    // Step 8: Enter main loop
    app.run();

    return 0;
}