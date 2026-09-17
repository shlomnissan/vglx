/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/core/identity.hpp"
#include "vglx/events/gamepad_event.hpp"
#include "vglx/events/keyboard_event.hpp"
#include "vglx/events/mouse_event.hpp"
#include "vglx/math/matrix3.hpp"
#include "vglx/math/transform2.hpp"
#include "vglx/math/vector2.hpp"

#include <concepts>
#include <memory>
#include <span>

namespace vglx {

class Canvas;

/**
 * @brief Base class for all 2D canvas nodes.
 *
 * Node2D represents a transformable object in a canvas hierarchy. It manages
 * local and world transforms, parent–child relationships, and hooks for per-frame
 * updates and input events. All 2D drawable objects, such as sprites and text,
 * derive from this class and share the same hierarchical behavior.
 *
 * The canvas is a tree rooted at a @ref Canvas node: each node can have zero or
 * more children and at most one parent. Transforms propagate downward, changing a
 * parent updates the world-space transforms of all descendants. Opacity is
 * inherited the same way, and hiding a node hides its entire subtree. Nodes are
 * drawn in tree order: parents before children, siblings in insertion order.
 * Input events propagate bottom-up through the hierarchy: children receive the
 * event before their parents, and any node can stop propagation by marking the
 * event as handled.
 *
 * @ingroup CanvasGroup
 */
class VGLX_EXPORT Node2D : public Identity {
public:
    /**
     * @brief Enumerates all node categories.
     *
     * Used by the engine and renderer to distinguish between drawable nodes,
     * the canvas root, and generic nodes. Custom node types typically reuse
     * one of the existing categories.
     */
    enum class Type {
        Default, ///< Generic node without special behavior.
        Canvas, ///< Root of a canvas hierarchy.
        Sprite ///< Textured quad drawn in canvas space.
    };

    /// @brief Local transform (position, rotation, scale, center) of this node.
    Transform2 transform {};

    /**
     * @brief Opacity of this node, where `1.0` is fully opaque.
     *
     * Multiplied by the opacity of every ancestor at draw time, so fading a
     * parent fades its whole subtree. Does not affect updates or input.
     */
    float opacity {1.0f};

    /**
     * @brief When `true` this node and its descendants are drawn.
     *
     * Hidden nodes are skipped by the renderer along with their entire subtree,
     * but still receive @ref OnUpdate calls.
     */
    bool visible {true};

    /**
     * @brief Constructs a node.
     */
    Node2D();

    /**
     * @brief Creates an instance of @ref Node2D.
     */
    [[nodiscard]] static auto Create() -> std::unique_ptr<Node2D> {
        return std::make_unique<Node2D>();
    }

    /**
     * @brief Returns the node's type identifier.
     *
     * Subclasses override this to report their specific type. The default is
     * @ref Node2D::Type "Node2D::Type::Default".
     */
    [[nodiscard]] virtual auto GetNodeType() const -> Node2D::Type {
        return Node2D::Type::Default;
    }

    /**
     * @brief Returns whether this node is renderable.
     *
     * Drawable subclasses, such as sprites, override this and return `true`.
     * The base implementation always returns `false`.
     */
    [[nodiscard]] virtual auto IsRenderable() const -> bool {
        return false;
    }

    /**
     * @name Hierarchy
     * @{
     */

    /**
     * @brief Adds a child node to this node and returns a non-owning reference.
     *
     * This overload transfers ownership of a node into this node’s children list.
     * The canvas is the sole owner of all nodes. The returned pointer is a non-owning
     * reference that remains valid only while the node is attached to the canvas.
     *
     * @param node Node to attach. Ownership is transferred.
     *
     * @warning The returned pointer becomes invalid if the node is removed from the
     * canvas or if the owning canvas is destroyed.
     */
    template <typename T>
    requires std::derived_from<T, Node2D>
    auto Add(std::unique_ptr<T> node) -> T* {
        return static_cast<T*>(AddImpl(std::unique_ptr<Node2D>(std::move(node))));
    }

    /**
     * @brief Detaches a direct child node from this node and returns ownership.
     *
     * Removes a node from this node’s children list without destroying it and
     * returns the owned subtree as a `std\::unique_ptr`. The detached node’s parent
     * pointer is cleared, its attached state is reset, and its transform is marked dirty.
     *
     * @param node Direct child node to detach.
     */
    template <typename T>
    requires std::derived_from<T, Node2D>
    [[nodiscard]] auto Detach(T* node) -> std::unique_ptr<T> {
        std::unique_ptr<Node2D> base = DetachImpl(node);
        return std::unique_ptr<T>(static_cast<T*>(base.release()));
    }

    /**
     * @brief Recursively updates world transforms for this node and its descendants.
     *
     * If the node’s local transform is dirty, or an ancestor was recomputed
     * during this pass, the world transform is recomputed from the parent’s
     * world transform (or from the local transform if this node is a root) and
     * the local transform is marked clean. The method then recurses into each child.
     *
     * This is the primary mechanism used by the renderer to update transform
     * propagation across the entire hierarchy once per frame.
     */
    auto UpdateTransformHierarchy() -> void {
        UpdateTransformHierarchyImpl(false);
    }

    /**
     * @brief Returns the world transform cached by the last hierarchy update.
     *
     * Reflects the state as of the most recent call to
     * @ref UpdateTransformHierarchy and may be stale if a transform changed
     * since. Intended for the renderer, which reads it after the per-frame
     * update. Use @ref GetWorldTransform for an always-current result.
     */
    [[nodiscard]] auto GetCachedWorldTransform() const -> const Matrix3&;

    /**
     * @brief Returns the node’s world transform matrix.
     *
     * Composes the local transforms along the parent chain without modifying
     * any cached state, so the result is current regardless of when the
     * hierarchy was last updated. The cost is proportional to the node’s depth.
     */
    [[nodiscard]] auto GetWorldTransform() const -> Matrix3;

    /**
     * @brief Returns the node’s world-space position.
     *
     * Computes the world transform via @ref GetWorldTransform, then extracts
     * the translation column of the matrix.
     */
    [[nodiscard]] auto GetWorldPosition() const -> Vector2;

    /**
     * @brief Removes a direct child node from this node and destroys it.
     *
     * If the node exists in the children list it is detached and destroyed.
     *
     * @param node Direct child node to remove.
     *
     * @warning Any external pointers or references to the removed node become invalid
     * immediately after this call.
     */
    auto Remove(Node2D* node) -> void;

    /**
     * @brief Removes all children from this node.
     *
     * Each child is detached from the canvas, has its parent pointer cleared,
     * its attached state reset, and its transform marked dirty. After all children
     * are processed, the children list is emptied.
     */
    auto RemoveAllChildren() -> void;

    /**
     * @brief Returns a view of this node’s direct children.
     *
     * The returned span exposes read-only access to the owning node's
     * `std\::unique_ptr<Node2D>` objects for each child. Ownership
     * is retained by the canvas.
     *
     * @note This function exposes the internal storage type used
     * by the canvas. It is intended for inspection and
     * iteration only.
     */
    [[nodiscard]] auto GetChildren() const -> std::span<const std::unique_ptr<Node2D>>;

    /**
     * @brief Returns a direct child node with the given name.
     *
     * Searches only this node's immediate children for a node whose name
     * matches the specified identifier. If no matching child is found,
     * returns `nullptr`.
     *
     * This function does not search descendants. Use @ref GetChildren
     * to implement recursive searches when needed.
     *
     * @param name Name of the child node to retrieve.
     */
    [[nodiscard]] auto GetChild(std::string_view name) const -> Node2D*;

    /**
     * @brief Checks whether the given node exists anywhere in this node’s subtree.
     *
     * @param node Node to test for membership in the subtree.
     */
    [[nodiscard]] auto IsChild(const Node2D* node) const -> bool;

    /**
     * @brief Returns this node’s parent.
     */
    [[nodiscard]] auto GetParent() const -> const Node2D*;

    /**
     * @brief Returns the canvas that owns this node.
     *
     * Returns the canvas this node is currently attached to or `nullptr`
     * if the node is not attached to any canvas.
     */
    [[nodiscard]] auto GetCanvas() const -> const Canvas*;

    /// @}

    /**
     * @name Event hooks
     * @{
     */

    /**
     * @brief Per-frame update callback.
     *
     * Called once per frame with the elapsed time since the last frame in
     * seconds. Override this to implement node behavior.
     *
     * @param delta Time step in seconds.
     */
    virtual auto OnUpdate(float delta) -> void {}

    /**
     * @brief Keyboard event handler.
     *
     * Override this to react to keyboard events. Events can be marked as
     * handled to stop propagation.
     *
     * @param event Keyboard event pointer.
     */
    virtual auto OnKeyboardEvent(KeyboardEvent* event) -> void {}

    /**
     * @brief Mouse event handler.
     *
     * Override this to react to cursor, button, or scroll events. Events can
     * be marked as handled to stop propagation.
     *
     * @param event Mouse event pointer.
     */
    virtual auto OnMouseEvent(MouseEvent* event) -> void {}

    /**
     * @brief Gamepad event handler.
     *
     * Override this to react to gamepad connection, button, or axis events.
     * Events can be marked as handled to stop propagation.
     *
     * @param event Gamepad event pointer.
     */
    virtual auto OnGamepadEvent(GamepadEvent* event) -> void {}

    /// @}

    virtual ~Node2D();

private:
    /// @cond INTERNAL
    class Impl;
    std::unique_ptr<Impl> impl_;

    friend class Canvas;

    auto AddImpl(std::unique_ptr<Node2D> node) -> Node2D*;

    auto DetachImpl(Node2D* node) -> std::unique_ptr<Node2D>;

    auto UpdateTransformHierarchyImpl(bool force_update) -> void;

    auto AttachSubtree(Canvas* canvas) -> void;

    auto DetachSubtree() -> void;
    /// @endcond
};

}
