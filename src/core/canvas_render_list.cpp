/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "core/canvas_render_list.hpp"

#include "utilities/assert.hpp"

namespace vglx {

auto CanvasRenderList::ProcessCanvas(Canvas* canvas) -> void {
    Reset();

    ProcessNode(canvas);
}

auto CanvasRenderList::ProcessNode(Node2D* node) -> void {
    VGLX_ASSERT(
        node != nullptr,
        "Failed to process canvas node: received null node"
    );

    if (!node->visible) return;

    if (node->IsRenderable()) renderables_.emplace_back(node);

    for (const auto& child : node->GetChildren()) {
        ProcessNode(child.get());
    }
}

auto CanvasRenderList::Reset() -> void {
    renderables_.clear();
}

}
