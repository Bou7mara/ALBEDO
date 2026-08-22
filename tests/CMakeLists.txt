

add_executable(rt_tests
    test_tuple3.cpp
    test_vector3.cpp
    test_point3.cpp
    test_normal3.cpp
    test_ray.cpp
    test_transform.cpp
    test_sphere.cpp
    test_vector2.cpp
    test_point2.cpp
    test_camera.cpp
    test_scene.cpp
    test_onb.cpp
    test_sampling.cpp
    test_lambertian.cpp
    test_shape.cpp
    test_rng.cpp
    test_metal.cpp
    test_bounds3.cpp
    test_bounds3_transform.cpp
    test_bvh.cpp
    test_dielectric.cpp
    test_fresnel.cpp
    test_microfacet.cpp
    test_microfacet_brdf.cpp
    test_emissive.cpp
    test_bsdf_pdf.cpp
    test_sphere_sampling.cpp
    test_mis_weights.cpp
    test_power_sampling.cpp
    test_progress.cpp
    test_multithreading.cpp
    test_triangle.cpp
    test_mesh_generators.cpp
    test_showcase.cpp
    test_obj_loader.cpp
    test_mesh_instance.cpp
    test_scene_node.cpp
    test_quad.cpp
    test_disney_principled.cpp
)

target_link_libraries(rt_tests PRIVATE rt_core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(rt_tests)
