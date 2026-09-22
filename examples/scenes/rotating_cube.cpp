/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <print>
#include <memory>

#include "example_runner.hpp"

struct Scene : public ExampleScene {
    vglx::Mesh* mesh {nullptr};

    Scene() {
        mesh = this->Add(vglx::Mesh::Create(
            vglx::BoxGeometry::Create(),
            vglx::PhongMaterial::Create({.color = 0x049EF4u})
        ));

        Add(vglx::AmbientLight::Create({
            .color = 0xFFFFFFu,
            .intensity = 0.5f
        }));

        Add(vglx::PointLight::Create({
            .color = 0xFFFFFFu,
            .intensity = 32.0f,
        }))->transform.Translate({2.0f, 2.5f, 4.0f});

        auto result = vglx::LoadFont(ASSETS_DIR "/fonts/futura_condensed_medium.fnt");
        if (!result.has_value()) {
            std::println("{}", result.error());
        }
    }

    auto OnUpdate(float dt) -> void override {
        mesh->transform.Rotate(vglx::Vector3::UnitX(), dt);
        mesh->transform.Rotate(vglx::Vector3::UnitY(), dt);
    }
};

auto get_camera() {
    auto camera = vglx::PerspectiveCamera::Create({
        .fov = vglx::math::DegToRad(60.0f),
        .aspect = static_cast<float>(kWindowWidth) / static_cast<float>(kWindowHeight),
        .near = 0.3f,
        .far = 1000.0f
    });

    return camera;
}

auto main() -> int {
    auto camera = get_camera();
    auto scene = std::make_unique<Scene>();

    scene.get()->Add(vglx::OrbitControls::Create(camera.get(), {
        .radius = 3.5f,
        .pitch = 0.25f
    }));

    return run_example(scene.get(), camera.get(), {
        .window_title = "Rotating Cube",
    });
}
