/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/canvas/canvas.hpp"
#include "vglx/canvas/node2d.hpp"

#include <span>
#include <vector>

namespace vglx {

class CanvasRenderList {
public:
    auto ProcessCanvas(Canvas* canvas) -> void;

    [[nodiscard]] auto Renderables() const -> std::span<Node2D* const> {
        return renderables_;
    }

private:
    std::vector<Node2D*> renderables_;

    auto ProcessNode(Node2D* node) -> void;

    auto Reset() -> void;
};

}
