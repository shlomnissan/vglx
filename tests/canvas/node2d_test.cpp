/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/canvas/canvas.hpp>
#include <vglx/canvas/node2d.hpp>

#pragma region Node Operations

TEST(Node2D, AddChild) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    EXPECT_EQ(parent->GetChildren().size(), 1);
    EXPECT_EQ(parent->GetChildren()[0].get(), child);
    EXPECT_EQ(child->GetParent(), parent.get());
}

TEST(Node2D, DetachChild) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    auto detached = parent->Detach(child);

    EXPECT_TRUE(parent->GetChildren().empty());
    EXPECT_EQ(detached.get(), child);
    EXPECT_EQ(detached->GetParent(), nullptr);
}

TEST(Node2D, RemoveChild) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->Remove(child);

    EXPECT_TRUE(parent->GetChildren().empty());
}

TEST(Node2D, RemoveAllChildren) {
    auto parent = vglx::Node2D::Create();

    parent->Add(vglx::Node2D::Create());
    parent->Add(vglx::Node2D::Create());
    parent->RemoveAllChildren();

    EXPECT_TRUE(parent->GetChildren().empty());
}

#pragma endregion

#pragma region Hierarchy Queries

TEST(Node2D, IsChild) {
    auto parent_0 = vglx::Node2D::Create();
    auto child_0 = parent_0->Add(vglx::Node2D::Create());

    auto parent_1 = vglx::Node2D::Create();
    auto child_1 = parent_1->Add(vglx::Node2D::Create());

    EXPECT_TRUE(parent_0->IsChild(child_0));
    EXPECT_FALSE(parent_0->IsChild(child_1));

    EXPECT_TRUE(parent_1->IsChild(child_1));
    EXPECT_FALSE(parent_1->IsChild(child_0));
}

TEST(Node2D, IsChildDescendant) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());
    auto grandchild = child->Add(vglx::Node2D::Create());

    EXPECT_TRUE(parent->IsChild(grandchild));
    EXPECT_TRUE(child->IsChild(grandchild));
    EXPECT_FALSE(grandchild->IsChild(parent.get()));
}

TEST(Node2D, IsChildAfterDetach) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    EXPECT_TRUE(parent->IsChild(child));

    auto detached = parent->Detach(child);
    EXPECT_FALSE(parent->IsChild(detached.get()));
}

TEST(Node2D, IsChildSelf) {
    auto node = vglx::Node2D::Create();
    EXPECT_FALSE(node->IsChild(node.get()));
}

TEST(Node2D, IsChildWithNullptr) {
    auto node = vglx::Node2D::Create();
    EXPECT_FALSE(node->IsChild(nullptr));
}

TEST(Node2D, GetChildByName) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());
    child->SetName("score");

    EXPECT_EQ(parent->GetChild("score"), child);
    EXPECT_EQ(parent->GetChild("missing"), nullptr);
}

#pragma endregion

#pragma region Update Transforms

TEST(Node2D, UpdateTransformsWithoutParent) {
    auto node = vglx::Node2D::Create();
    node->transform.SetScale({2.0f, 2.0f});

    node->UpdateTransformHierarchy();

    EXPECT_MAT3_EQ(node->GetCachedWorldTransform(), {
        2.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    });
}

TEST(Node2D, UpdateTransformsWithParent) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->transform.SetScale({2.0f, 2.0f});
    parent->UpdateTransformHierarchy();

    EXPECT_MAT3_EQ(child->GetCachedWorldTransform(), {
        2.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    });
}

TEST(Node2D, UpdateTransformsComposeParentFirst) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->transform.SetPosition({5.0f, 0.0f});
    child->transform.SetPosition({1.0f, 2.0f});
    parent->UpdateTransformHierarchy();

    EXPECT_MAT3_EQ(child->GetCachedWorldTransform(), {
        1.0f, 0.0f, 6.0f,
        0.0f, 1.0f, 2.0f,
        0.0f, 0.0f, 1.0f
    });
}

TEST(Node2D, UpdateTransformsPropagateToCleanChildren) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->UpdateTransformHierarchy();
    parent->transform.SetPosition({5.0f, 0.0f});
    parent->UpdateTransformHierarchy();

    EXPECT_MAT3_EQ(child->GetCachedWorldTransform(), {
        1.0f, 0.0f, 5.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    });
}

TEST(Node2D, MarkTransformedNodeAsTouched) {
    auto node = vglx::Node2D::Create();
    node->UpdateTransformHierarchy();

    node->transform.SetScale({0.5f, 0.5f});

    EXPECT_TRUE(node->transform.touched);
}

TEST(Node2D, MarkTransformedNodeAsUntouched) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->UpdateTransformHierarchy();

    EXPECT_FALSE(parent->transform.touched);
    EXPECT_FALSE(child->transform.touched);
}

TEST(Node2D, MarkDetachedNodesAsTouched) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->UpdateTransformHierarchy();

    auto detached = parent->Detach(child);

    EXPECT_TRUE(detached->transform.touched);
}

#pragma endregion

#pragma region World Transform Queries

TEST(Node2D, WorldTransformWithDirtyAncestor) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->UpdateTransformHierarchy();
    parent->transform.SetPosition({5.0f, 0.0f});

    // The query is current without a hierarchy update...
    EXPECT_MAT3_EQ(child->GetWorldTransform(), {
        1.0f, 0.0f, 5.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    });

    // ...while the cache still reflects the last update.
    EXPECT_MAT3_EQ(child->GetCachedWorldTransform(), {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    });
}

TEST(Node2D, WorldTransformQueryPreservesDirtyState) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());
    auto grandchild = child->Add(vglx::Node2D::Create());

    parent->UpdateTransformHierarchy();
    parent->transform.SetPosition({3.0f, 0.0f});

    (void)parent->GetWorldTransform();
    EXPECT_TRUE(parent->transform.touched);

    parent->UpdateTransformHierarchy();

    EXPECT_MAT3_EQ(grandchild->GetCachedWorldTransform(), {
        1.0f, 0.0f, 3.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    });
    EXPECT_FALSE(parent->transform.touched);
    EXPECT_FALSE(child->transform.touched);
    EXPECT_FALSE(grandchild->transform.touched);
}

TEST(Node2D, WorldPositionWithParent) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->transform.SetPosition({5.0f, 0.0f});
    child->transform.SetPosition({1.0f, 2.0f});

    EXPECT_VEC2_EQ(child->GetWorldPosition(), {6.0f, 2.0f});
}

TEST(Node2D, WorldPositionWithRotatedParent) {
    auto parent = vglx::Node2D::Create();
    auto child = parent->Add(vglx::Node2D::Create());

    parent->transform.SetRotation(vglx::math::pi_over_2);
    child->transform.SetPosition({1.0f, 0.0f});

    EXPECT_VEC2_NEAR(child->GetWorldPosition(), {0.0f, 1.0f}, 0.001f);
}

#pragma endregion

#pragma region Canvas Attachment

TEST(Node2D, GetCanvasWithoutCanvas) {
    auto node = vglx::Node2D::Create();
    EXPECT_EQ(node->GetCanvas(), nullptr);
}

TEST(Node2D, GetCanvasAfterAttach) {
    auto canvas = vglx::Canvas::Create();
    auto child = canvas->Add(vglx::Node2D::Create());
    auto grandchild = child->Add(vglx::Node2D::Create());

    EXPECT_EQ(child->GetCanvas(), canvas.get());
    EXPECT_EQ(grandchild->GetCanvas(), canvas.get());
}

TEST(Node2D, GetCanvasAfterDetach) {
    auto canvas = vglx::Canvas::Create();
    auto child = canvas->Add(vglx::Node2D::Create());
    auto grandchild = child->Add(vglx::Node2D::Create());

    auto detached = canvas->Detach(child);

    EXPECT_EQ(detached->GetCanvas(), nullptr);
    EXPECT_EQ(grandchild->GetCanvas(), nullptr);
}

#pragma endregion

#pragma region Assertions

TEST(Node2D, DeathWhenAddingNullptr) {
#ifdef NDEBUG
    GTEST_SKIP() << "VGLX_ASSERT is disabled in release builds";
#endif

    auto parent = vglx::Node2D::Create();

    EXPECT_DEATH({
        parent->Add(std::unique_ptr<vglx::Node2D> {});
    }, ".*nullptr");
}

TEST(Node2D, DeathWhenAddingAncestor) {
#ifdef NDEBUG
    GTEST_SKIP() << "VGLX_ASSERT is disabled in release builds";
#endif

    auto root = vglx::Node2D::Create();
    auto parent = root->Add(vglx::Node2D::Create());
    auto child = parent->Add(vglx::Node2D::Create());

    auto detached = root->Detach(parent);

    EXPECT_DEATH({
        child->Add(std::move(detached));
    }, ".*ancestor");
}

TEST(Node2D, DeathWhenDetachingNonChild) {
#ifdef NDEBUG
    GTEST_SKIP() << "VGLX_ASSERT is disabled in release builds";
#endif

    auto parent = vglx::Node2D::Create();
    auto child = vglx::Node2D::Create();

    EXPECT_DEATH({
        parent->Remove(child.get());
    }, ".*not a child");
}

#pragma endregion
