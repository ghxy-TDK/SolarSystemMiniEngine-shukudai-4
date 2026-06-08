// raytracing/whitted.h
// Whitted-style ray tracer (Stage 10).
// Zero OpenGL dependency. Zero third-party library dependency.
// All inputs are plain C++ value types.
//
// ---------------------------------------------------------------
// TRIGGER PROTOCOL (implemented in Stage 11):
//
//   static bool g_raytrace_pending = false;  // set in keyboard cb
//
//   glutKeyboardFunc handler:
//     case 'r': case 'R':
//         g_raytrace_pending = true;
//         break;
//
//   End of display callback (before glutSwapBuffers):
//     if (g_raytrace_pending) {
//         g_raytrace_pending = false;
//         // freeze: stop posting redraws until done
//         std::cout << "Raytracing... please wait\n";
//         Whitted::Params p;
//         p.width      = window_width;
//         p.height     = window_height;
//         p.max_depth  = 4;
//         p.eye        = camera.get_position();
//         p.lookat     = camera.get_target();
//         p.up         = Vec3(0.f, 1.f, 0.f);
//         p.fov_deg    = camera.get_fov();
//         Whitted::render(solar_system.get_scene_data_for_raytracing(),
//                         p, "screenshots/raytrace.bmp");
//         std::cout << "Done. Saved to screenshots/raytrace.bmp\n";
//         glutPostRedisplay();  // resume
//     }
// ---------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../math/vec3.h"
#include "ray.h"
#include "intersection.h"
#include "../scene/solar_system.h"

// Hard-coded light for the ray tracer: always positioned at the
// origin (the Sun).  A full light list could be passed via Params
// if needed in future stages.
struct RtLight {
    Vec3  position;
    Vec3  color;
    float intensity;
};

// ------------------------------------------------------------
// Whitted
// Static-only class: all state is stack/heap-local per render.
// ------------------------------------------------------------
class Whitted {
public:
    // All parameters needed to describe a render job.
    struct Params {
        int   width     = 800;
        int   height    = 600;
        int   max_depth = 4;       // reflection recursion limit
        Vec3  eye;                 // camera world position
        Vec3  lookat;              // look-at target
        Vec3  up;                  // world up (typically 0,1,0)
        float fov_deg   = 45.f;   // vertical field of view

        // Light baked into params so the class stays self-contained.
        // Defaults to a white sun at the origin.
        RtLight light = { Vec3(0.f, 0.f, 0.f),
                          Vec3(1.f, 1.f, 1.f),
                          1.5f };

        // Ambient scale [0,1] applied to every hit.
        float ambient_intensity = 0.08f;

        // Mirror reflectance applied when shininess >= threshold.
        float reflection_threshold = 64.f;
        float reflection_strength  = 0.35f;
    };

    // Entry point.
    // Blocks until the image is fully rendered and written to disk.
    static void render(const std::vector<RenderableSphere>& scene,
                       const Params&                        params,
                       const std::string&                   output_path);

private:
    // Recursively traces one ray; returns the computed color.
    // depth counts down from params.max_depth to 0.
    static Vec3 trace(const Ray&                           ray,
                      const std::vector<RenderableSphere>& scene,
                      const Params&                        params,
                      int                                  depth);

    // Ray-sphere analytic intersection.
    // Returns an Intersection with hit=true and t > k_epsilon on a hit.
    static Intersection intersect_sphere(const Ray&            ray,
                                         const RenderableSphere& sphere);

    // Nearest hit across the whole scene.
    // Returns hit=false when no sphere is intersected.
    static Intersection intersect_scene(const Ray&                           ray,
                                        const std::vector<RenderableSphere>& scene,
                                        int*                                 out_index);

    // Shadow test: is the hit point in shadow w.r.t. the light?
    static bool in_shadow(const Vec3&                          point,
                          const Vec3&                          light_pos,
                          const std::vector<RenderableSphere>& scene,
                          int                                  self_index);

    // Writes a 24-bit uncompressed BMP to disk.
    // rgb layout: row-major, top-to-bottom, 3 bytes per pixel (R,G,B).
    // Creates parent directories if they do not exist.
    static void save_bmp(int                          w,
                         int                          h,
                         const std::vector<uint8_t>&  rgb,
                         const std::string&           path);

    // Small offset to push shadow / reflection rays off the surface.
    static constexpr float k_epsilon = 1e-4f;
};
