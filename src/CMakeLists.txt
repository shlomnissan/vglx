set(PUBLIC_HEADERS_DIR ${CMAKE_SOURCE_DIR}/include/vglx)
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

set(SOURCE_CODE
    "cameras/camera.cpp"
    "cameras/orthographic_camera.cpp"
    "cameras/perspective_camera.cpp"
    "core/identity.cpp"
    "core/program_attributes.cpp"
    "core/program_attributes.hpp"
    "core/render_lists.cpp"
    "core/render_lists.hpp"
    "core/render_target.cpp"
    "core/renderer.cpp"
    "core/shader_library.cpp"
    "core/shader_library.hpp"
    "core/window.cpp"
    "core/window_impl.cpp"
    "core/window_impl.hpp"
    "events/event_dispatcher.hpp"
    "geometries/buffer_attribute.cpp"
    "geometries/generate_attributes.cpp"
    "geometries/generate_attributes.hpp"
    "geometries/vertex_streams.cpp"
    "geometries/vertex_streams.hpp"
    "geometries/geometry.cpp"
    "geometries/wireframe_geometry.cpp"
    "helpers/arrow.cpp"
    "helpers/bounding_box.cpp"
    "helpers/bounding_plane.cpp"
    "helpers/bounding_sphere.cpp"
    "helpers/grid.cpp"
    "helpers/orbit_controls.cpp"
    "lights/directional_light.cpp"
    "lights/point_light.cpp"
    "lights/spot_light.cpp"
    "loaders/detail/gltf_import.cpp"
    "loaders/detail/gltf_import.hpp"
    "loaders/detail/image_import.cpp"
    "loaders/detail/image_import.hpp"
    "loaders/detail/obj_import.cpp"
    "loaders/detail/obj_import.hpp"
    "loaders/detail/texture_ref.hpp"
    "loaders/load_cube_texture.cpp"
    "loaders/load_image.cpp"
    "loaders/load_mesh.cpp"
    "loaders/load_texture.cpp"
    "materials/shader_material.cpp"
    "primitives/box_geometry.cpp"
    "primitives/cone_geometry.cpp"
    "primitives/cylinder_geometry.cpp"
    "primitives/plane_geometry.cpp"
    "primitives/sphere_geometry.cpp"
    "primitives/torus_knot_geometry.cpp"
    "renderer/gl/gl_background_pass.cpp"
    "renderer/gl/gl_background_pass.hpp"
    "renderer/gl/gl_binding_state.cpp"
    "renderer/gl/gl_binding_state.hpp"
    "renderer/gl/gl_device.cpp"
    "renderer/gl/gl_device.hpp"
    "renderer/gl/gl_environment.cpp"
    "renderer/gl/gl_environment.hpp"
    "renderer/gl/gl_framebuffers.cpp"
    "renderer/gl/gl_framebuffers.hpp"
    "renderer/gl/gl_lights.cpp"
    "renderer/gl/gl_lights.hpp"
    "renderer/gl/gl_present_pass.cpp"
    "renderer/gl/gl_present_pass.hpp"
    "renderer/gl/gl_program.cpp"
    "renderer/gl/gl_program.hpp"
    "renderer/gl/gl_programs.cpp"
    "renderer/gl/gl_programs.hpp"
    "renderer/gl/gl_renderer_impl.cpp"
    "renderer/gl/gl_renderer_impl.hpp"
    "renderer/gl/gl_scene_buffer.cpp"
    "renderer/gl/gl_scene_buffer.hpp"
    "renderer/gl/gl_shadow_maps.cpp"
    "renderer/gl/gl_shadow_maps.hpp"
    "renderer/gl/gl_state.cpp"
    "renderer/gl/gl_state.hpp"
    "renderer/gl/gl_textures.cpp"
    "renderer/gl/gl_textures.hpp"
    "renderer/gl/gl_uniform.cpp"
    "renderer/gl/gl_uniform.hpp"
    "renderer/gl/gl_uniform_buffer.cpp"
    "renderer/gl/gl_uniform_buffer.hpp"
    "renderer/gl/gl_buffers.cpp"
    "renderer/gl/gl_buffers.hpp"
    "scene/billboard.cpp"
    "scene/instanced_mesh.cpp"
    "scene/mesh.cpp"
    "scene/node.cpp"
    "scene/renderable.cpp"
    "scene/scene.cpp"
    "textures/dynamic_texture_2d.cpp"
    "utilities/assert.hpp"
    "utilities/data_series.hpp"
    "utilities/hash.hpp"
    "utilities/logger.cpp"
    "utilities/logger.hpp"
    "utilities/scoped_timer.hpp"
    "utilities/stats.cpp"
    "utilities/thread_pool.cpp"
    "utilities/thread_pool.hpp"
    "utilities/timer.cpp"
)

set(PUBLIC_HEADERS
    "${PUBLIC_HEADERS_DIR}/cameras/camera.hpp"
    "${PUBLIC_HEADERS_DIR}/cameras/orthographic_camera.hpp"
    "${PUBLIC_HEADERS_DIR}/cameras/perspective_camera.hpp"
    "${PUBLIC_HEADERS_DIR}/core/disposable.hpp"
    "${PUBLIC_HEADERS_DIR}/core/identity.hpp"
    "${PUBLIC_HEADERS_DIR}/core/render_target.hpp"
    "${PUBLIC_HEADERS_DIR}/core/renderer.hpp"
    "${PUBLIC_HEADERS_DIR}/core/window.hpp"
    "${PUBLIC_HEADERS_DIR}/events/event.hpp"
    "${PUBLIC_HEADERS_DIR}/events/keyboard_event.hpp"
    "${PUBLIC_HEADERS_DIR}/events/mouse_event.hpp"
    "${PUBLIC_HEADERS_DIR}/geometries/buffer_attribute.hpp"
    "${PUBLIC_HEADERS_DIR}/geometries/geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/geometries/wireframe_geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/helpers/arrow.hpp"
    "${PUBLIC_HEADERS_DIR}/helpers/bounding_box.hpp"
    "${PUBLIC_HEADERS_DIR}/helpers/bounding_plane.hpp"
    "${PUBLIC_HEADERS_DIR}/helpers/bounding_sphere.hpp"
    "${PUBLIC_HEADERS_DIR}/helpers/grid.hpp"
    "${PUBLIC_HEADERS_DIR}/helpers/orbit_controls.hpp"
    "${PUBLIC_HEADERS_DIR}/lights/ambient_light.hpp"
    "${PUBLIC_HEADERS_DIR}/lights/directional_light.hpp"
    "${PUBLIC_HEADERS_DIR}/lights/light.hpp"
    "${PUBLIC_HEADERS_DIR}/lights/point_light.hpp"
    "${PUBLIC_HEADERS_DIR}/lights/spot_light.hpp"
    "${PUBLIC_HEADERS_DIR}/materials/billboard_material.hpp"
    "${PUBLIC_HEADERS_DIR}/materials/depth_material.hpp"
    "${PUBLIC_HEADERS_DIR}/materials/material.hpp"
    "${PUBLIC_HEADERS_DIR}/materials/pbr_material.hpp"
    "${PUBLIC_HEADERS_DIR}/materials/phong_material.hpp"
    "${PUBLIC_HEADERS_DIR}/materials/shader_material.hpp"
    "${PUBLIC_HEADERS_DIR}/materials/unlit_material.hpp"
    "${PUBLIC_HEADERS_DIR}/math/box3.hpp"
    "${PUBLIC_HEADERS_DIR}/math/color.hpp"
    "${PUBLIC_HEADERS_DIR}/math/constants.hpp"
    "${PUBLIC_HEADERS_DIR}/math/detail/tables.hpp"
    "${PUBLIC_HEADERS_DIR}/math/euler.hpp"
    "${PUBLIC_HEADERS_DIR}/math/frustum.hpp"
    "${PUBLIC_HEADERS_DIR}/math/matrix3.hpp"
    "${PUBLIC_HEADERS_DIR}/math/matrix4.hpp"
    "${PUBLIC_HEADERS_DIR}/math/plane.hpp"
    "${PUBLIC_HEADERS_DIR}/math/quaternion.hpp"
    "${PUBLIC_HEADERS_DIR}/math/sphere.hpp"
    "${PUBLIC_HEADERS_DIR}/math/spherical.hpp"
    "${PUBLIC_HEADERS_DIR}/math/transform2.hpp"
    "${PUBLIC_HEADERS_DIR}/math/transform3.hpp"
    "${PUBLIC_HEADERS_DIR}/math/utilities.hpp"
    "${PUBLIC_HEADERS_DIR}/math/vector2.hpp"
    "${PUBLIC_HEADERS_DIR}/math/vector3.hpp"
    "${PUBLIC_HEADERS_DIR}/math/vector4.hpp"
    "${PUBLIC_HEADERS_DIR}/primitives/box_geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/primitives/cone_geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/primitives/cylinder_geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/primitives/plane_geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/primitives/sphere_geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/primitives/torus_knot_geometry.hpp"
    "${PUBLIC_HEADERS_DIR}/scene/billboard.hpp"
    "${PUBLIC_HEADERS_DIR}/scene/fog.hpp"
    "${PUBLIC_HEADERS_DIR}/scene/instanced_mesh.hpp"
    "${PUBLIC_HEADERS_DIR}/scene/mesh.hpp"
    "${PUBLIC_HEADERS_DIR}/scene/node.hpp"
    "${PUBLIC_HEADERS_DIR}/scene/renderable.hpp"
    "${PUBLIC_HEADERS_DIR}/scene/scene.hpp"
    "${PUBLIC_HEADERS_DIR}/textures/cube_texture.hpp"
    "${PUBLIC_HEADERS_DIR}/textures/dynamic_texture_2d.hpp"
    "${PUBLIC_HEADERS_DIR}/textures/image.hpp"
    "${PUBLIC_HEADERS_DIR}/textures/texture.hpp"
    "${PUBLIC_HEADERS_DIR}/textures/texture_2d.hpp"
    "${PUBLIC_HEADERS_DIR}/utilities/frame_timer.hpp"
    "${PUBLIC_HEADERS_DIR}/utilities/logging.hpp"
    "${PUBLIC_HEADERS_DIR}/utilities/stats.hpp"
    "${PUBLIC_HEADERS_DIR}/utilities/timer.hpp"
)

set(HEADER_BUNDLES
    "${PUBLIC_HEADERS_DIR}/cameras.hpp"
    "${PUBLIC_HEADERS_DIR}/core.hpp"
    "${PUBLIC_HEADERS_DIR}/events.hpp"
    "${PUBLIC_HEADERS_DIR}/geometries.hpp"
    "${PUBLIC_HEADERS_DIR}/helpers.hpp"
    "${PUBLIC_HEADERS_DIR}/lights.hpp"
    "${PUBLIC_HEADERS_DIR}/loaders.hpp"
    "${PUBLIC_HEADERS_DIR}/materials.hpp"
    "${PUBLIC_HEADERS_DIR}/math.hpp"
    "${PUBLIC_HEADERS_DIR}/primitives.hpp"
    "${PUBLIC_HEADERS_DIR}/scene.hpp"
    "${PUBLIC_HEADERS_DIR}/textures.hpp"
    "${PUBLIC_HEADERS_DIR}/utilities.hpp"
    "${PUBLIC_HEADERS_DIR}/vglx.hpp"
)

include(ShaderProcessor)

add_library(
    ${PROJECT_NAME}
    ${PUBLIC_HEADERS}
    ${HEADER_BUNDLES}
    ${SOURCE_CODE}
    ${VENDOR_SOURCES}
)

# disable exceptions
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fno-exceptions>
    $<$<CXX_COMPILER_ID:Clang>:-fno-exceptions>
    $<$<CXX_COMPILER_ID:AppleClang>:-fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${VENDOR_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)

target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw)

if (VGLX_BUILD_IMGUI)
    set(IMGUI_SOURCES
        ${VENDOR_DIR}/imgui/imgui.cpp
        ${VENDOR_DIR}/imgui/imgui_draw.cpp
        ${VENDOR_DIR}/imgui/imgui_tables.cpp
        ${VENDOR_DIR}/imgui/imgui_widgets.cpp
        ${VENDOR_DIR}/imgui/backends/imgui_impl_glfw.cpp
        ${VENDOR_DIR}/imgui/backends/imgui_impl_opengl3.cpp
    )

    target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES} core/imgui_integration.hpp)

    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE "IMGUI_API=__declspec(dllexport)")
    else()
        set_source_files_properties(${IMGUI_SOURCES} PROPERTIES COMPILE_FLAGS "-fvisibility=default")
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE
        $<BUILD_INTERFACE:${VENDOR_DIR}/imgui>
    )

    target_compile_definitions(${PROJECT_NAME} PUBLIC VGLX_USE_IMGUI=ON)
endif()

if(VGLX_LOG_LEVEL STREQUAL "")
    set(vglx_log_level "$<IF:$<CONFIG:Debug>,3,1>")
else()
    set(vglx_log_level "${VGLX_LOG_LEVEL}")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE VGLX_LOG_LEVEL=${vglx_log_level})

set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    VERSION ${vglx_VERSION_MAJOR}.${vglx_VERSION_MINOR}.${vglx_VERSION_PATCH}
    SOVERSION ${vglx_VERSION_MAJOR}
)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/version.hpp
    @ONLY
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${vglx_VERSION_MAJOR}.${vglx_VERSION_MINOR}.${vglx_VERSION_PATCH}
    COMPATIBILITY SameMinorVersion
)

set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
        COMPONENT ${PROJECT_NAME}_runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
        COMPONENT ${PROJECT_NAME}_runtime
        NAMELINK_COMPONENT ${PROJECT_NAME}_development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${PUBLIC_HEADERS_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.hpp"
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vglx_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/version.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CONFIG_INSTALL_DIR}
    COMPONENT development
)

if (VGLX_BUILD_IMGUI)
    install(DIRECTORY ${VENDOR_DIR}/imgui/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/imgui
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "backends" EXCLUDE
    )
endif()

install(
    EXPORT ${PROJECT_NAME}
    DESTINATION ${CONFIG_INSTALL_DIR}
    NAMESPACE ${PROJECT_NAME}::
)
