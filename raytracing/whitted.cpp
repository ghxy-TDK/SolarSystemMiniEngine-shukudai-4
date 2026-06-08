// raytracing/whitted.cpp
// Whitted-style ray tracer implementation.
// Dependencies: C++17 standard library only + project math library.
// No OpenGL, no third-party image library.

#include "whitted.h"

#include <algorithm>   // std::min / std::max / std::clamp
#include <cassert>
#include <cmath>
#include <cstdio>      // std::fopen / std::fwrite / std::fclose
#include <cstring>     // std::memset
#include <filesystem>  // std::filesystem::create_directories
#include <iostream>    // std::cout progress

#include "../math/math_utils.h"  // deg_to_rad (or we compute inline)

// ----------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------
namespace {

    // Clamp a float to [lo, hi].
    inline float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    // Convert linear float color [0,1] to uint8_t with gamma 2.2.
    inline uint8_t to_byte(float c) {
        c = clampf(c, 0.f, 1.f);
        c = std::pow(c, 1.f / 2.2f);          // gamma correction
        return static_cast<uint8_t>(c * 255.f + 0.5f);
    }

    // Component-wise multiply two Vec3 values.
    inline Vec3 mul_comp(const Vec3& a, const Vec3& b) {
        return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
    }

    // Reflect incident direction i around normal n.
    // Both vectors should point away from the surface.
    inline Vec3 reflect(const Vec3& i, const Vec3& n) {
        // r = i - 2*(i.n)*n  (i points toward surface, so sign convention:
        // here we treat i as the incoming ray direction, pointing TOWARD surface)
        float cos_theta = i.dot(n);
        return Vec3(i.x - 2.f * cos_theta * n.x,
            i.y - 2.f * cos_theta * n.y,
            i.z - 2.f * cos_theta * n.z);
    }

    // Degrees to radians.
    inline float deg2rad(float d) {
        return d * 3.14159265358979323846f / 180.f;
    }

    // Portable fopen: keeps #ifdef out of function bodies to avoid MSVC
    // brace-matching issues in certain project configurations.
    inline std::FILE* portable_fopen(const char* path, const char* mode) {
        std::FILE* fp = nullptr;
#ifdef _MSC_VER
        fopen_s(&fp, path, mode);
#else
        fp = std::fopen(path, mode);
#endif
        return fp;
    }

} // anonymous namespace

// ----------------------------------------------------------------
// intersect_sphere
// Analytic ray-sphere test.
// Ray: P(t) = origin + t*direction
// Sphere: |P - center|^2 = radius^2
// Solves the quadratic; returns nearest positive t > k_epsilon.
// ----------------------------------------------------------------
Intersection Whitted::intersect_sphere(const Ray& ray,
    const RenderableSphere& sphere)
{
    Intersection rec;
    // Vector from sphere center to ray origin.
    Vec3 oc = Vec3(ray.origin.x - sphere.center.x,
        ray.origin.y - sphere.center.y,
        ray.origin.z - sphere.center.z);

    float a = ray.direction.dot(ray.direction);
    float b = 2.f * oc.dot(ray.direction);
    float c = oc.dot(oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.f * a * c;

    if (discriminant < 0.f) {
        return rec; // miss
    }

    float sqrt_d = std::sqrt(discriminant);
    float t0 = (-b - sqrt_d) / (2.f * a);
    float t1 = (-b + sqrt_d) / (2.f * a);

    float t = -1.f;
    if (t0 > k_epsilon) {
        t = t0;
    }
    else if (t1 > k_epsilon) {
        t = t1; // inside the sphere
    }

    if (t < 0.f) {
        return rec; // both roots behind ray origin
    }

    rec.hit = true;
    rec.t = t;
    rec.point = ray.at(t);

    // Outward unit normal.
    Vec3 n = Vec3((rec.point.x - sphere.center.x) / sphere.radius,
        (rec.point.y - sphere.center.y) / sphere.radius,
        (rec.point.z - sphere.center.z) / sphere.radius);
    rec.normal = n;
    return rec;
}

// ----------------------------------------------------------------
// intersect_scene
// Tests all spheres; returns the nearest hit.
// *out_index receives the index of the hit sphere, or -1 on miss.
// ----------------------------------------------------------------
Intersection Whitted::intersect_scene(const Ray& ray,
    const std::vector<RenderableSphere>& scene,
    int* out_index)
{
    Intersection nearest;
    nearest.t = std::numeric_limits<float>::max();
    *out_index = -1;

    for (int i = 0; i < static_cast<int>(scene.size()); ++i) {
        Intersection rec = intersect_sphere(ray, scene[i]);
        if (rec.hit && rec.t < nearest.t) {
            nearest = rec;
            *out_index = i;
        }
    }
    return nearest;
}

// ----------------------------------------------------------------
// in_shadow
// Returns true when a shadow ray from `point` toward `light_pos`
// is blocked by any sphere other than `self_index`.
// ----------------------------------------------------------------
bool Whitted::in_shadow(const Vec3& point,
    const Vec3& light_pos,
    const std::vector<RenderableSphere>& scene,
    int                                  self_index)
{
    Vec3 to_light = Vec3(light_pos.x - point.x,
        light_pos.y - point.y,
        light_pos.z - point.z);
    float dist_to_light = to_light.length();

    // Avoid division by zero (light at the surface, shouldn't happen).
    if (dist_to_light < k_epsilon) return false;

    Vec3 dir = Vec3(to_light.x / dist_to_light,
        to_light.y / dist_to_light,
        to_light.z / dist_to_light);
    Ray shadow_ray;
    shadow_ray.origin = Vec3(point.x + dir.x * k_epsilon,
        point.y + dir.y * k_epsilon,
        point.z + dir.z * k_epsilon);
    shadow_ray.direction = dir;

    for (int i = 0; i < static_cast<int>(scene.size()); ++i) {
        if (i == self_index) continue;
        Intersection rec = intersect_sphere(shadow_ray, scene[i]);
        if (rec.hit && rec.t < dist_to_light) {
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------
// trace
// Recursive Whitted trace.
// Shading model: Phong (ambient + diffuse + specular) + reflection.
// ----------------------------------------------------------------
Vec3 Whitted::trace(const Ray& ray,
    const std::vector<RenderableSphere>& scene,
    const Params& params,
    int depth)
{
    Vec3 bg_top(0.00f, 0.00f, 0.02f);
    Vec3 bg_bot(0.00f, 0.00f, 0.00f);

    int hit_index = -1;
    Intersection rec = intersect_scene(ray, scene, &hit_index);

    if (rec.hit && hit_index == 0) {
        return Vec3(1.f, 0.95f, 0.7f);
    }

    if (!rec.hit) {
        float t = clampf(ray.direction.normalized().y * 0.5f + 0.5f, 0.f, 1.f);
        return Vec3(bg_bot.x + t * (bg_top.x - bg_bot.x),
            bg_bot.y + t * (bg_top.y - bg_bot.y),
            bg_bot.z + t * (bg_top.z - bg_bot.z));
    }

    if (rec.hit) {
        const RenderableSphere& s = scene[hit_index];
        if (s.shininess == 0.f) {
            return Vec3(s.color.x * 2.f, s.color.y * 2.f, s.color.z * 1.5f);
        }
    }

    const RenderableSphere& sphere = scene[hit_index];
    Vec3 N = rec.normal;

    // --- 方案二：半球环境光替代纯 ambient ---
    float sky_factor = clampf(N.y * 0.5f + 0.5f, 0.f, 1.f);
    Vec3  sky_color = Vec3(0.1f, 0.15f, 0.3f);
    Vec3  ground_color = Vec3(0.05f, 0.05f, 0.05f);
    Vec3 ambient = Vec3(
        sphere.color.x * (sky_color.x * sky_factor + ground_color.x * (1.f - sky_factor)) * 2.f,
        sphere.color.y * (sky_color.y * sky_factor + ground_color.y * (1.f - sky_factor)) * 2.f,
        sphere.color.z * (sky_color.z * sky_factor + ground_color.z * (1.f - sky_factor)) * 2.f
    );

    // --- Light vector and shadow test ---
    Vec3 to_light = Vec3(params.light.position.x - rec.point.x,
        params.light.position.y - rec.point.y,
        params.light.position.z - rec.point.z);
    float dist2 = to_light.dot(to_light);
    float dist = std::sqrt(dist2);
    Vec3  L = (dist > k_epsilon)
        ? Vec3(to_light.x / dist, to_light.y / dist, to_light.z / dist)
        : Vec3(0.f, 1.f, 0.f);

    float n_dot_l = clampf(N.dot(L), 0.f, 1.f);
    bool  shadowed = in_shadow(rec.point, params.light.position, scene, hit_index);
    float attenuation = params.light.intensity / (1.f + 0.05f * dist2);

    // --- Diffuse ---
    Vec3 diffuse(0.f, 0.f, 0.f);
    if (!shadowed) {
        float diff = n_dot_l * attenuation;
        diffuse = Vec3(sphere.color.x * params.light.color.x * diff,
            sphere.color.y * params.light.color.y * diff,
            sphere.color.z * params.light.color.z * diff);
    }

    // --- Specular ---
    Vec3 specular(0.f, 0.f, 0.f);
    if (!shadowed && n_dot_l > 0.f) {
        Vec3 V = Vec3(params.eye.x - rec.point.x,
            params.eye.y - rec.point.y,
            params.eye.z - rec.point.z).normalized();
        float n_dot_l2 = N.dot(L);
        Vec3  R = Vec3(2.f * n_dot_l2 * N.x - L.x,
            2.f * n_dot_l2 * N.y - L.y,
            2.f * n_dot_l2 * N.z - L.z);
        float rv = clampf(R.dot(V), 0.f, 1.f);
        float spec = std::pow(rv, sphere.shininess) * attenuation;
        Vec3 spec_color(0.8f + 0.2f * sphere.color.x,
            0.8f + 0.2f * sphere.color.y,
            0.8f + 0.2f * sphere.color.z);
        specular = Vec3(spec_color.x * spec,
            spec_color.y * spec,
            spec_color.z * spec);
    }

    // --- 方案三：侧面补光 ---
    Vec3  fill_dir = Vec3(1.f, 0.5f, 0.f).normalized();
    float fill_dot = clampf(N.dot(fill_dir), 0.f, 1.f);
    Vec3  fill_light(
        sphere.color.x * 0.15f * fill_dot,
        sphere.color.y * 0.15f * fill_dot,
        sphere.color.z * 0.15f * fill_dot
    );

    // --- Combine ---
    Vec3 local(
        clampf(ambient.x + diffuse.x + specular.x + fill_light.x, 0.f, 1.f),
        clampf(ambient.y + diffuse.y + specular.y + fill_light.y, 0.f, 1.f),
        clampf(ambient.z + diffuse.z + specular.z + fill_light.z, 0.f, 1.f)
    );

    // --- Reflection ---
    if (depth > 0 && sphere.shininess >= params.reflection_threshold) {
        Vec3  I = ray.direction.normalized();
        float i_dot_n = I.dot(N);
        Vec3  refl_dir(I.x - 2.f * i_dot_n * N.x,
            I.y - 2.f * i_dot_n * N.y,
            I.z - 2.f * i_dot_n * N.z);
        Ray refl_ray;
        refl_ray.origin = Vec3(rec.point.x + N.x * k_epsilon,
            rec.point.y + N.y * k_epsilon,
            rec.point.z + N.z * k_epsilon);
        refl_ray.direction = refl_dir.normalized();
        Vec3  refl_color = trace(refl_ray, scene, params, depth - 1);
        float s = params.reflection_strength;
        local = Vec3(local.x * (1.f - s) + refl_color.x * s,
            local.y * (1.f - s) + refl_color.y * s,
            local.z * (1.f - s) + refl_color.z * s);
    }

    return local;
}

// ----------------------------------------------------------------
// save_bmp
// Writes a 24-bit uncompressed BMP (no compression, no palette).
// Pixel rows are stored bottom-up as per the BMP specification,
// so we flip vertically when writing.
// Row stride must be padded to a multiple of 4 bytes.
// ----------------------------------------------------------------
void Whitted::save_bmp(int                         w,
    int                         h,
    const std::vector<uint8_t>& rgb,   // R,G,B top-to-bottom
    const std::string& path)
{
    // Create parent directory (C++17).
    {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    }

    // Row size padded to 4 bytes; BMP stores BGR.
    int row_stride = (w * 3 + 3) & ~3;  // round up to multiple of 4
    int pixel_bytes = row_stride * h;
    int file_size = 14 + 40 + pixel_bytes;  // file header + info header + data

    // --- File header (14 bytes) ---
    uint8_t file_hdr[14];
    std::memset(file_hdr, 0, sizeof(file_hdr));
    file_hdr[0] = 'B'; file_hdr[1] = 'M';
    // File size (little-endian 32-bit).
    file_hdr[2] = static_cast<uint8_t>(file_size & 0xFF);
    file_hdr[3] = static_cast<uint8_t>((file_size >> 8) & 0xFF);
    file_hdr[4] = static_cast<uint8_t>((file_size >> 16) & 0xFF);
    file_hdr[5] = static_cast<uint8_t>((file_size >> 24) & 0xFF);
    // Pixel data offset = 14 + 40 = 54.
    file_hdr[10] = 54;

    // --- Info header (BITMAPINFOHEADER, 40 bytes) ---
    uint8_t info_hdr[40];
    std::memset(info_hdr, 0, sizeof(info_hdr));
    // Header size.
    info_hdr[0] = 40;
    // Width (little-endian 32-bit).
    info_hdr[4] = static_cast<uint8_t>(w & 0xFF);
    info_hdr[5] = static_cast<uint8_t>((w >> 8) & 0xFF);
    info_hdr[6] = static_cast<uint8_t>((w >> 16) & 0xFF);
    info_hdr[7] = static_cast<uint8_t>((w >> 24) & 0xFF);
    // Height (positive = bottom-up, which is what we write).
    info_hdr[8] = static_cast<uint8_t>(h & 0xFF);
    info_hdr[9] = static_cast<uint8_t>((h >> 8) & 0xFF);
    info_hdr[10] = static_cast<uint8_t>((h >> 16) & 0xFF);
    info_hdr[11] = static_cast<uint8_t>((h >> 24) & 0xFF);
    // Color planes = 1.
    info_hdr[12] = 1;
    // Bits per pixel = 24.
    info_hdr[14] = 24;
    // Pixel data size.
    info_hdr[20] = static_cast<uint8_t>(pixel_bytes & 0xFF);
    info_hdr[21] = static_cast<uint8_t>((pixel_bytes >> 8) & 0xFF);
    info_hdr[22] = static_cast<uint8_t>((pixel_bytes >> 16) & 0xFF);
    info_hdr[23] = static_cast<uint8_t>((pixel_bytes >> 24) & 0xFF);

    // --- Write ---
    std::FILE* fp = portable_fopen(path.c_str(), "wb");
    if (!fp) {
        std::cerr << "[Whitted] Failed to open output file: " << path << "\n";
        return;
    }

    std::fwrite(file_hdr, 1, 14, fp);
    std::fwrite(info_hdr, 1, 40, fp);

    // Pixel rows: BMP is bottom-up, our rgb buffer is top-down.
    std::vector<uint8_t> row_buf(static_cast<size_t>(row_stride), 0);
    for (int row = h - 1; row >= 0; --row) {
        for (int col = 0; col < w; ++col) {
            int src = (row * w + col) * 3;
            int dst = col * 3;
            row_buf[dst + 0] = rgb[src + 2]; // B
            row_buf[dst + 1] = rgb[src + 1]; // G
            row_buf[dst + 2] = rgb[src + 0]; // R
        }
        // Padding bytes are already zeroed.
        std::fwrite(row_buf.data(), 1, static_cast<size_t>(row_stride), fp);
    }

    std::fclose(fp);
}

// ----------------------------------------------------------------
// render
// Builds the camera basis, shoots primary rays, writes the BMP.
// ----------------------------------------------------------------
void Whitted::render(const std::vector<RenderableSphere>& scene,
    const Params& params,
    const std::string& output_path)
{
    assert(params.width > 0);
    assert(params.height > 0);
    assert(params.max_depth >= 0);

    const int W = params.width;
    const int H = params.height;

    // --- Camera basis vectors ---
    Vec3 forward = Vec3(params.lookat.x - params.eye.x,
        params.lookat.y - params.eye.y,
        params.lookat.z - params.eye.z).normalized();
    Vec3 right = forward.cross(params.up).normalized();
    Vec3 up_cam = right.cross(forward);  // orthonormalized up

    // half-height of the image plane at unit distance.
    float half_h = std::tan(deg2rad(params.fov_deg * 0.5f));
    float half_w = half_h * (static_cast<float>(W) / static_cast<float>(H));

    // Pixel buffer: R,G,B interleaved, top-to-bottom, left-to-right.
    std::vector<uint8_t> rgb(static_cast<size_t>(W * H * 3), 0);

    // --- Primary ray loop ---
    int total_pixels = W * H;
    int progress_step = total_pixels / 20;  // print every 5%
    if (progress_step < 1) progress_step = 1;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int pixel_index = y * W + x;
            if (pixel_index % progress_step == 0) {
                int pct = (pixel_index * 100) / total_pixels;
                std::cout << "[Whitted] " << pct << "%\r" << std::flush;
            }

            // NDC coordinates in [-1, 1].
            float ndc_x = (static_cast<float>(x) + 0.5f) / static_cast<float>(W);
            float ndc_y = (static_cast<float>(y) + 0.5f) / static_cast<float>(H);
            // Remap to [-1, 1]; y is flipped (image row 0 = top).
            float screen_x = (2.f * ndc_x - 1.f) * half_w;
            float screen_y = -(2.f * ndc_y - 1.f) * half_h;

            // Ray direction in world space.
            Vec3 dir = Vec3(forward.x + screen_x * right.x + screen_y * up_cam.x,
                forward.y + screen_x * right.y + screen_y * up_cam.y,
                forward.z + screen_x * right.z + screen_y * up_cam.z)
                .normalized();

            Ray primary;
            primary.origin = params.eye;
            primary.direction = dir;

            Vec3 color = trace(primary, scene, params, params.max_depth);

            int dst = pixel_index * 3;
            rgb[dst + 0] = to_byte(color.x);
            rgb[dst + 1] = to_byte(color.y);
            rgb[dst + 2] = to_byte(color.z);
        }
    }

    std::cout << "[Whitted] 100% - writing " << output_path << "\n";
    save_bmp(W, H, rgb, output_path);
}