#include "material.h"
#include "shader.h"

void Material::apply(Shader& shader) const {
    shader.set_vec3 ("u_material.ambient",   ambient);
    shader.set_vec3 ("u_material.diffuse",   diffuse);
    shader.set_vec3 ("u_material.specular",  specular);
    shader.set_float("u_material.shininess", shininess);
}
