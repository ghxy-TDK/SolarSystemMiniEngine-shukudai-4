#version 130

// ------------------------------------------------------------
// Inputs from vertex shader
// ------------------------------------------------------------
in vec3 v_frag_pos;   // world-space fragment position
in vec3 v_normal;     // world-space normal (normalised in vert)
in vec2 v_tex_coord;  // UV (unused for now, reserved)

// ------------------------------------------------------------
// Uniforms  -- names from project table sec 0.4
// ------------------------------------------------------------
uniform int   u_lighting_model;   // 0=Ambient 1=Lambert 2=Phong 3=Blinn

struct MaterialData {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};
uniform MaterialData u_material;

struct PointLightData {
    vec3  position;
    vec3  color;
    float intensity;
};
uniform PointLightData u_point_light;

uniform vec3 u_cam_pos;

// ------------------------------------------------------------
// Output
// ------------------------------------------------------------
out vec4 frag_color;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

// Returns the diffuse (Lambert) contribution.
// n and l must be unit vectors.
vec3 lambert_diffuse(vec3 n, vec3 l, vec3 light_radiance) {
    float n_dot_l = max(dot(n, l), 0.0);
    return u_material.diffuse * light_radiance * n_dot_l;
}

// Returns the Phong specular contribution (uses reflect()).
// n, l, v must be unit vectors.
vec3 phong_specular(vec3 n, vec3 l, vec3 v, vec3 light_radiance) {
    vec3  r       = reflect(-l, n);
    float r_dot_v = max(dot(r, v), 0.0);
    float spec    = pow(r_dot_v, u_material.shininess);
    return u_material.specular * light_radiance * spec;
}

// Returns the Blinn-Phong specular contribution (uses half-vector).
// n, l, v must be unit vectors.
vec3 blinn_phong_specular(vec3 n, vec3 l, vec3 v, vec3 light_radiance) {
    vec3  h       = normalize(l + v);
    float n_dot_h = max(dot(n, h), 0.0);
    float spec    = pow(n_dot_h, u_material.shininess);
    return u_material.specular * light_radiance * spec;
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
void main() {
    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_point_light.position - v_frag_pos);
    vec3 v = normalize(u_cam_pos - v_frag_pos);

    // Effective radiance of the light at this fragment.
    vec3 light_radiance = u_point_light.color * u_point_light.intensity;

    vec3 result = vec3(0.0);

    // Mode 0: Ambient only
    if (u_lighting_model == 0) {
        result = u_material.ambient;
    }
    // Mode 1: Lambert (diffuse only, no specular)
    else if (u_lighting_model == 1) {
        vec3 ambient  = u_material.ambient;
        vec3 diffuse  = lambert_diffuse(n, l, light_radiance);
        result = ambient + diffuse;
    }
    // Mode 2: Phong (diffuse + specular via reflect())
    else if (u_lighting_model == 2) {
        vec3 ambient  = u_material.ambient;
        vec3 diffuse  = lambert_diffuse(n, l, light_radiance);
        vec3 specular = phong_specular(n, l, v, light_radiance);
        result = ambient + diffuse + specular;
    }
    // Mode 3: Blinn-Phong (diffuse + specular via half-vector)
    else {
        vec3 ambient  = u_material.ambient;
        vec3 diffuse  = lambert_diffuse(n, l, light_radiance);
        vec3 specular = blinn_phong_specular(n, l, v, light_radiance);
        result = ambient + diffuse + specular;
    }

    frag_color = vec4(result, 1.0);
}
