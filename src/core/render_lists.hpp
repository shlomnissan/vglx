/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/cameras/camera.hpp"
#include "vglx/lights/light.hpp"
#include "vglx/math/frustum.hpp"
#include "vglx/scene/node.hpp"
#include "vglx/scene/renderable.hpp"
#include "vglx/scene/scene.hpp"

#include <memory>
#include <span>
#include <vector>

namespace vglx {

class RenderLists {
public:
    auto ProcessScene(Scene* scene, Camera* camera, bool sort = true) -> void;

    [[nodiscard]] auto Opaque() const -> std::span<Renderable* const> {
        return opaque_;
    }

    [[nodiscard]] auto Transparent() const -> std::span<Renderable* const> {
        return transparent_;
    }

    [[nodiscard]] auto Lights() const -> std::span<Light* const> {
        return lights_;
    }

private:
    std::vector<Renderable*> opaque_;

    std::vector<Renderable*> transparent_;

    std::vector<Light*> lights_;

    auto ProcessNode(Node* node, const Frustum& frustum) -> void;

    auto Reset() -> void;
};

}
