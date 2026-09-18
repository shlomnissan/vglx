/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/canvas/node2d.hpp"

#include "vglx/canvas/canvas.hpp"

#include "utilities/assert.hpp"

#include <algorithm>
#include <vector>

namespace vglx {

struct Node2D::Impl {
    std::vector<std::unique_ptr<Node2D>> children;
    Canvas* canvas {nullptr};
    Node2D* parent {nullptr};
    Matrix3 world_transform {1.0f};
};

Node2D::Node2D() : impl_(std::make_unique<Impl>()) {}

auto Node2D::AddImpl(std::unique_ptr<Node2D> node) -> Node2D* {
    auto raw = node.get();

    VGLX_ASSERT(
        raw != nullptr,
        "Failed to add node2d: node is set to nullptr"
    );

    VGLX_ASSERT(
        raw != this,
        "Failed to add node2d: node cannot be added to itself"
    );

    VGLX_ASSERT(
        raw->impl_->parent == nullptr,
        "Failed to add node2d: node already has a parent"
    );

    VGLX_ASSERT(
        !raw->IsChild(this),
        "Failed to add node2d: node is an ancestor"
    );

    impl_->children.emplace_back(std::move(node));

    raw->impl_->parent = this;
    if (impl_->canvas) {
        raw->AttachSubtree(impl_->canvas);
    }

    return raw;
}

auto Node2D::DetachImpl(Node2D* node) -> std::unique_ptr<Node2D> {
    VGLX_ASSERT(
        node != nullptr,
        "Failed to detach node2d: node is set to nullptr"
    );

    auto it = std::ranges::find_if(impl_->children, [node](const auto& child){
        return child.get() == node;
    });

    VGLX_ASSERT(
        it != impl_->children.end(),
        "Failed to detach node2d: node is not a child"
    );

    auto out = std::move(*it);
    impl_->children.erase(it);

    out->DetachSubtree();
    out->impl_->parent = nullptr;
    out->transform.touched = true;

    return out;
}

auto Node2D::AttachSubtree(Canvas* canvas) -> void {
    if (impl_->canvas) return;
    impl_->canvas = canvas;
    for (const auto& child : impl_->children) {
        child->AttachSubtree(canvas);
    }
}

auto Node2D::DetachSubtree() -> void {
    if (!impl_->canvas) return;
    impl_->canvas = nullptr;
    transform.touched = true;
    for (auto& child : impl_->children) child->DetachSubtree();
}

auto Node2D::Remove(Node2D* node) -> void {
    (void)DetachImpl(node);
}

auto Node2D::RemoveAllChildren() -> void {
    for (auto& child : impl_->children) {
        child->DetachSubtree();
        child->impl_->parent = nullptr;
        child->transform.touched = true;
    }
    impl_->children.clear();
}

auto Node2D::UpdateTransformHierarchyImpl(bool force_update) -> void {
    if (transform.touched || force_update) {
        impl_->world_transform = impl_->parent == nullptr
            ? transform.Get()
            : impl_->parent->impl_->world_transform * transform.Get();

        force_update = true;
    }

    for (const auto& child : GetChildren()) {
        child->UpdateTransformHierarchyImpl(force_update);
    }
}

auto Node2D::GetWorldTransform() const -> Matrix3 {
    const auto local = transform.Get();
    return impl_->parent == nullptr ? local : impl_->parent->GetWorldTransform() * local;
}

auto Node2D::GetCachedWorldTransform() const -> const Matrix3& {
    return impl_->world_transform;
}

auto Node2D::GetWorldPosition() const -> Vector2 {
    const auto t = GetWorldTransform()[2];
    return Vector2 {t.x, t.y};
}

auto Node2D::GetWorldOpacity() const -> float {
    return impl_->parent == nullptr
        ? opacity
        : impl_->parent->GetWorldOpacity() * opacity;
}

auto Node2D::GetChildren() const -> std::span<const std::unique_ptr<Node2D>> {
    return impl_->children;
}

auto Node2D::GetChild(std::string_view name) const -> Node2D* {
    for (const auto& child : impl_->children) {
        if (child->Name() == name) return child.get();
    }
    return nullptr;
}

auto Node2D::IsChild(const Node2D* node) const -> bool {
    while (node != nullptr) {
        node = node->impl_->parent;
        if (node == this) return true;
    }
    return false;
}

auto Node2D::GetParent() const -> const Node2D* {
    return impl_->parent;
}

auto Node2D::GetCanvas() const -> const Canvas* {
    return impl_->canvas;
}

Node2D::~Node2D() = default;

}
