#pragma once
#include "../math/vec3.h"

class Shader;  // forward declaration avoids pulling in full header here

// ---------------------------------------------------------------------------
// Material
//
// Holds Phong shading coefficients and applies them to a Shader via the
// uniform names defined in section 0.4:
//   u_material.ambient   vec3
//   u_material.diffuse   vec3
//   u_material.specular  vec3
//   u_material.shininess float
// ---------------------------------------------------------------------------
struct Material {
    Vec3  ambient   = {0.1f, 0.1f, 0.1f};
    Vec3  diffuse   = {0.8f, 0.8f, 0.8f};
    Vec3  specular  = {0.5f, 0.5f, 0.5f};
    float shininess = 32.f;

    // Uploads all material uniforms to the currently bound shader.
    // Caller is responsible for calling shader.use() beforehand.
    void apply(Shader& shader) const;
};
