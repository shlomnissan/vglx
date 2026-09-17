/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/scene/node.hpp"

#include "vglx/scene/scene.hpp"

#include "utilities/assert.hpp"

#include <algorithm>
#include <vector>

namespace vglx {

struct Node::Impl {
    std::vector<std::unique_ptr<Node>> children;
    Scene* scene {nullptr};
    Node* parent {nullptr};
    Matrix4 world_transform {1.0f};
};

Node::Node() : impl_(std::make_unique<Impl>()) {}

auto Node::AddImpl(std::unique_ptr<Node> node) -> Node* {
    auto raw = node.get();

    VGLX_ASSERT(
        raw != nullptr,
        "Failed to add node: node is set to nullptr"
    );

    VGLX_ASSERT(
        raw != this,
        "Failed to add node: node cannot be added to itself"
    );

    VGLX_ASSERT(
        raw->impl_->parent == nullptr,
        "Failed to add node: node already has a parent"
    );

    VGLX_ASSERT(
        !raw->IsChild(this),
        "Failed to add node: node is an ancestor"
    );

    impl_->children.emplace_back(std::move(node));

    raw->impl_->parent = this;
    if (impl_->scene) {
        raw->AttachSubtree(impl_->scene);
    }

    return raw;
}

auto Node::DetachImpl(Node* node) -> std::unique_ptr<Node> {
    VGLX_ASSERT(
        node != nullptr,
        "Failed to detach node: node is set to nullptr"
    );

    auto it = std::ranges::find_if(impl_->children, [node](const auto& child){
        return child.get() == node;
    });

    VGLX_ASSERT(
        it != impl_->children.end(),
        "Failed to detach node: node is not a child"
    );

    auto out = std::move(*it);
    impl_->children.erase(it);

    out->DetachSubtree();
    out->impl_->parent = nullptr;
    out->transform.touched = true;

    return out;
}

auto Node::AttachSubtree(Scene* scene) -> void {
    if (impl_->scene) return;
    impl_->scene = scene;
    for (const auto& child : impl_->children) {
        child->AttachSubtree(scene);
    }
}

auto Node::DetachSubtree() -> void {
    if (!impl_->scene) return;
    impl_->scene = nullptr;
    transform.touched = true;
    for (auto& child : impl_->children) child->DetachSubtree();
}

auto Node::Remove(Node* node) -> void {
    (void)DetachImpl(node);
}

auto Node::RemoveAllChildren() -> void {
    for (auto& child : impl_->children) {
        child->DetachSubtree();
        child->impl_->parent = nullptr;
        child->transform.touched = true;
    }
    impl_->children.clear();
}

auto Node::UpdateTransformHierarchyImpl(bool force_update) -> void {
    const auto update = transform_auto_update && (transform.touched || force_update);
    if (update) {
        impl_->world_transform = impl_->parent == nullptr
            ? transform.Get()
            : impl_->parent->impl_->world_transform * transform.Get();
    }

    for (const auto& child : impl_->children) {
        child->UpdateTransformHierarchyImpl(update);
    }
}

auto Node::GetWorldTransform() const -> Matrix4 {
    if (!transform_auto_update) {
        return impl_->world_transform;
    }

    const auto local = transform.Get();
    return impl_->parent == nullptr ? local : impl_->parent->GetWorldTransform() * local;
}

auto Node::GetCachedWorldTransform() const -> const Matrix4& {
    return impl_->world_transform;
}

auto Node::GetWorldPosition() const -> Vector3 {
    const auto t = GetWorldTransform()[3];
    return Vector3 {t.x, t.y, t.z};
}

auto Node::GetChildren() const -> std::span<const std::unique_ptr<Node>> {
    return impl_->children;
}

auto Node::GetChild(std::string_view name) const -> Node* {
    for (const auto& child : impl_->children) {
        if (child->Name() == name) return child.get();
    }
    return nullptr;
}

auto Node::IsChild(const Node* node) const -> bool {
    while (node != nullptr) {
        node = node->impl_->parent;
        if (node == this) return true;
    }
    return false;
}

auto Node::GetParent() const -> const Node* {
    return impl_->parent;
}

auto Node::GetScene() const -> const Scene* {
    return impl_->scene;
}

auto Node::LookAt(const Vector3& target) -> void {
    transform.LookAt(GetWorldPosition(), target, up);
}

Node::~Node() = default;

}
