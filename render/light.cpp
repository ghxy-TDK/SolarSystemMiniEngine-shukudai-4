#include "light.h"
#include "shader.h"

// Uploads all u_point_light.* uniforms to the bound shader.
// Uniform names are taken verbatim from the project uniform table (sec 0.4).
void PointLight::apply(Shader& shader) const {
    shader.set_vec3 ("u_point_light.position",  position);
    shader.set_vec3 ("u_point_light.color",     color);
    shader.set_float("u_point_light.intensity", intensity);
}
