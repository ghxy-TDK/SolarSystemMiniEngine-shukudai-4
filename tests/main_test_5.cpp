#include <cassert>
#include <cstdio>
#include <cmath>
#include <memory>
#include <string>

#include "../scene/transform.h"
#include "../render/material.h"
#include "../scene/object.h"
#include "../scene/scene.h"
#include "../geometry/mesh.h"

static bool approx(float a, float b, float eps = 1e-4f) {
    float d = a - b; return (d < 0 ? -d : d) < eps;
}
static void pass(const char* n) { std::printf("  [PASS] %s\n", n); }

static void test_transform_identity() {
    Transform t; Matrix4 m = t.get_local_matrix();
    assert(approx(m.m[0],1.f)); assert(approx(m.m[5],1.f));
    assert(approx(m.m[10],1.f)); assert(approx(m.m[15],1.f));
    pass("transform_identity");
}
static void test_transform_translate() {
    Transform t; t.position = {3.f,5.f,7.f};
    Matrix4 m = t.get_local_matrix();
    assert(approx(m.m[12],3.f)); assert(approx(m.m[13],5.f)); assert(approx(m.m[14],7.f));
    pass("transform_translate");
}
static void test_transform_scale() {
    Transform t; t.scale = {2.f,3.f,4.f};
    Matrix4 m = t.get_local_matrix();
    assert(approx(m.m[0],2.f)); assert(approx(m.m[5],3.f)); assert(approx(m.m[10],4.f));
    pass("transform_scale");
}
static void test_transform_rotate_90_y() {
    Transform t; t.rotation_axis={0.f,1.f,0.f}; t.rotation_angle_deg=90.f;
    Matrix4 m = t.get_local_matrix();
    assert(approx(m.m[0],0.f,1e-3f)); assert(approx(m.m[2],-1.f,1e-3f));
    pass("transform_rotate_90_y");
}
static void test_transform_parent_child() {
    Transform parent; parent.position={10.f,0.f,0.f};
    Transform child;  child.position={5.f,0.f,0.f}; child.parent=&parent;
    Matrix4 w = child.get_world_matrix();
    assert(approx(w.m[12],15.f));
    pass("transform_parent_child");
}
static void test_transform_no_parent() {
    Transform t; t.position={1.f,2.f,3.f};
    Matrix4 l=t.get_local_matrix(), w=t.get_world_matrix();
    for (int i=0;i<16;++i) assert(approx(l.m[i],w.m[i]));
    pass("transform_no_parent");
}
static void test_transform_deep_chain() {
    Transform gp; gp.position={1.f,0.f,0.f};
    Transform par; par.position={2.f,0.f,0.f}; par.parent=&gp;
    Transform ch;  ch.position={3.f,0.f,0.f};  ch.parent=&par;
    assert(approx(ch.get_world_matrix().m[12],6.f));
    pass("transform_deep_chain");
}

static void test_material_defaults() {
    Material mat;
    assert(approx(mat.shininess,32.f)); assert(approx(mat.ambient.x,0.1f));
    assert(approx(mat.diffuse.x,0.8f)); assert(approx(mat.specular.x,0.5f));
    pass("material_defaults");
}
static void test_material_field_assignment() {
    Material mat;
    mat.ambient={0.2f,0.3f,0.4f}; mat.diffuse={0.5f,0.6f,0.7f};
    mat.specular={0.8f,0.9f,1.0f}; mat.shininess=64.f;
    assert(approx(mat.ambient.x,0.2f)); assert(approx(mat.diffuse.y,0.6f));
    assert(approx(mat.specular.z,1.0f)); assert(approx(mat.shininess,64.f));
    pass("material_field_assignment");
}
static void test_material_shared_ptr() {
    auto mat=std::make_shared<Material>(); mat->shininess=128.f;
    auto o1=std::make_unique<Object>("a"); auto o2=std::make_unique<Object>("b");
    o1->material=mat; o2->material=mat;
    o1->material->shininess=256.f;
    assert(approx(o2->material->shininess,256.f));
    pass("material_shared_ptr");
}

static void test_object_name() { Object o("earth"); assert(o.name=="earth"); pass("object_name"); }
static void test_object_default_material() { Object o; assert(o.material!=nullptr); pass("object_default_material"); }
static void test_object_add_mesh() {
    Object o("t"); assert(o.mesh_count()==0u);
    std::vector<Vertex> v(3); std::vector<unsigned int> idx={0,1,2};
    o.add_mesh(std::make_unique<Mesh>(v,idx)); assert(o.mesh_count()==1u);
    o.add_mesh(std::make_unique<Mesh>(v,idx)); assert(o.mesh_count()==2u);
    pass("object_add_mesh");
}
static void test_object_transform_world_pos() {
    Object o("p"); o.transform.position={3.f,0.f,0.f};
    assert(approx(o.transform.get_world_matrix().m[12],3.f));
    pass("object_transform_world_pos");
}
static void test_object_material_values() {
    Object o("sun"); o.material->diffuse={1.f,0.9f,0.f}; o.material->shininess=8.f;
    assert(approx(o.material->diffuse.x,1.f)); assert(approx(o.material->shininess,8.f));
    pass("object_material_values");
}
static void test_object_null_material_guard() {
    Object o; o.material=nullptr;
    assert(o.material==nullptr); assert(o.mesh_count()==0u);
    pass("object_null_material_guard");
}

static void test_scene_empty() {
    Scene s;
    assert(s.object_count()==0u); assert(s.light_count()==0u); assert(s.particle_system_count()==0u);
    pass("scene_empty");
}
static void test_scene_add_objects() {
    Scene s; s.add_object(std::make_unique<Object>("sun")); s.add_object(std::make_unique<Object>("earth"));
    assert(s.object_count()==2u);
    pass("scene_add_objects");
}
static void test_scene_get_objects_non_owning() {
    Scene s; s.add_object(std::make_unique<Object>("mercury"));
    auto v=s.get_objects(); assert(v.size()==1u); assert(v[0]!=nullptr); assert(v[0]->name=="mercury");
    pass("scene_get_objects_non_owning");
}
static void test_scene_null_add_ignored() {
    Scene s; s.add_object(nullptr); assert(s.object_count()==0u);
    pass("scene_null_add_ignored");
}
static void test_scene_update_no_crash() { Scene s; s.update(0.016f); pass("scene_update_no_crash"); }
static void test_scene_distinct_ptrs() {
    Scene s;
    for (int i=0;i<9;++i) s.add_object(std::make_unique<Object>("p"+std::to_string(i)));
    auto v=s.get_objects(); assert(v.size()==9u);
    for (std::size_t i=0;i<v.size();++i) { assert(v[i]!=nullptr); for (std::size_t j=i+1;j<v.size();++j) assert(v[i]!=v[j]); }
    pass("scene_distinct_ptrs");
}
static void test_scene_lifetime() {
    Scene s; s.add_object(std::make_unique<Object>("jupiter"));
    Object* raw=s.get_objects()[0]; assert(raw->name=="jupiter");
    s.add_object(std::make_unique<Object>("saturn"));
    assert(raw->name=="jupiter");
    pass("scene_lifetime");
}

static void test_integration_scaffold() {
    Scene scene;
    auto sun=std::make_unique<Object>("sun");
    sun->transform.scale={2.f,2.f,2.f}; sun->material->shininess=8.f;
    auto earth=std::make_unique<Object>("earth");
    earth->transform.position={5.f,0.f,0.f};
    scene.add_object(std::move(sun)); scene.add_object(std::move(earth));
    assert(scene.object_count()==2u);
    for (Object* o : scene.get_objects()) {
        if (o->name=="sun")   assert(approx(o->transform.get_world_matrix().m[0],2.f));
        if (o->name=="earth") assert(approx(o->transform.get_world_matrix().m[12],5.f));
    }
    pass("integration_scaffold");
}
static void test_integration_parent_child() {
    Object sun("sun"); Object earth("earth");
    earth.transform.position={8.f,0.f,0.f}; earth.transform.parent=&sun.transform;
    assert(approx(earth.transform.get_world_matrix().m[12],8.f));
    pass("integration_parent_child");
}

int main() {
    std::printf("=== Stage 5: Transform / Material / Object / Scene ===\n");
    std::printf("\n-- Transform --\n");
    test_transform_identity(); test_transform_translate(); test_transform_scale();
    test_transform_rotate_90_y(); test_transform_parent_child();
    test_transform_no_parent(); test_transform_deep_chain();
    std::printf("\n-- Material --\n");
    test_material_defaults(); test_material_field_assignment(); test_material_shared_ptr();
    std::printf("\n-- Object --\n");
    test_object_name(); test_object_default_material(); test_object_add_mesh();
    test_object_transform_world_pos(); test_object_material_values(); test_object_null_material_guard();
    std::printf("\n-- Scene --\n");
    test_scene_empty(); test_scene_add_objects(); test_scene_get_objects_non_owning();
    test_scene_null_add_ignored(); test_scene_update_no_crash();
    test_scene_distinct_ptrs(); test_scene_lifetime();
    std::printf("\n-- Integration --\n");
    test_integration_scaffold(); test_integration_parent_child();
    std::printf("\n=== All Stage 5 tests passed ===\n");
    return 0;
}
