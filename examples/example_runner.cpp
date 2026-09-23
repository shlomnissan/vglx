/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "example_runner.hpp"

#include <print>

auto run_example(ExampleScene* scene, vglx::Camera* camera, const ExampleSettings& settings) -> int {
    auto window = vglx::Window {{
        .title = settings.window_title,
        .width = kWindowWidth,
        .height = kWindowHeight,
        .vsync = true
    }};

    if (auto result = window.Initialize(); !result.has_value()) {
        std::println(stderr, "{}", result.error());
        return 1;
    }

    auto renderer = vglx::Renderer {{
        .sample_count = kSampleCount,
        .clear_color = settings.clear_color,
        .tone_mapping = settings.tone_mapping,
        .exposure = settings.exposure,
        .shadow_map = settings.shadow_map
    }};

    if (auto result = renderer.Initialize(window); !result.has_value()) {
        std::println(stderr, "{}", result.error());
        return 1;
    }

    window.OnResize([&](const vglx::ResizeParameters& params){
        camera->Resize(params.window_width, params.window_height);
        scene->canvas.Resize(params.window_width, params.window_height);
    });

    auto timer = vglx::FrameTimer {true};

    #ifdef VGLX_EXAMPLES_ENABLE_UI
        auto stats = vglx::Stats {};
    #endif

    while(!window.ShouldClose()) {
        window.PollEvents();
        scene->Advance(timer.Tick());

    #ifdef VGLX_EXAMPLES_ENABLE_UI
        stats.BeforeRender();
        renderer.Render(scene, camera);
        stats.AfterRender(renderer.RenderedObjectsPerFrame());

        window.BeginUIFrame();
        stats.Draw();
        scene->OnDrawUI();
        window.EndUIFrame();
    #else
        renderer.Render(scene, camera);
    #endif

        window.SwapBuffers();
    }

    return 0;
}
