/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/math/utilities.hpp>
#include <vglx/scene/node.hpp>
#include <vglx/scene/scene.hpp>

#pragma region Node Operations

TEST(Node, AddChild) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    EXPECT_EQ(parent->GetChildren().size(), 1);
    EXPECT_EQ(parent->GetChildren()[0].get(), child);
    EXPECT_EQ(child->GetParent(), parent.get());
}

TEST(Node, DetachChild) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    auto detached = parent->Detach(child);

    EXPECT_TRUE(parent->GetChildren().empty());
    EXPECT_EQ(detached.get(), child);
    EXPECT_EQ(detached->GetParent(), nullptr);
}

TEST(Node, RemoveChild) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->Remove(child);

    EXPECT_TRUE(parent->GetChildren().empty());
}

TEST(Node, RemoveAllChildren) {
    auto parent = vglx::Node::Create();

    parent->Add(vglx::Node::Create());
    parent->Add(vglx::Node::Create());
    parent->RemoveAllChildren();

    EXPECT_TRUE(parent->GetChildren().empty());
}

#pragma endregion

#pragma region Hierarchy Queries

TEST(Node, IsChild) {
    auto parent_0 = vglx::Node::Create();
    auto child_0 = parent_0->Add(vglx::Node::Create());

    auto parent_1 = vglx::Node::Create();
    auto child_1 = parent_1->Add(vglx::Node::Create());

    EXPECT_TRUE(parent_0->IsChild(child_0));
    EXPECT_FALSE(parent_0->IsChild(child_1));

    EXPECT_TRUE(parent_1->IsChild(child_1));
    EXPECT_FALSE(parent_1->IsChild(child_0));
}

TEST(Node, IsChildDescendant) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());
    auto grandchild = child->Add(vglx::Node::Create());

    EXPECT_TRUE(parent->IsChild(grandchild));
    EXPECT_TRUE(child->IsChild(grandchild));
    EXPECT_FALSE(grandchild->IsChild(parent.get()));
}

TEST(Node, IsChildAfterDetach) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    EXPECT_TRUE(parent->IsChild(child));

    auto detached = parent->Detach(child);
    EXPECT_FALSE(parent->IsChild(detached.get()));
}

TEST(Node, IsChildSelf) {
    auto node = vglx::Node::Create();
    EXPECT_FALSE(node->IsChild(node.get()));
}

TEST(Node, IsChildWithNullptr) {
    auto node = vglx::Node::Create();
    EXPECT_FALSE(node->IsChild(nullptr));
}

TEST(Node, GetChildByName) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());
    child->SetName("light");

    EXPECT_EQ(parent->GetChild("light"), child);
    EXPECT_EQ(parent->GetChild("missing"), nullptr);
}

#pragma endregion

#pragma region Update Transforms

TEST(Node, UpdateTransformsWithoutParent) {
    auto node = vglx::Node::Create();
    node->transform.SetScale(2.0f);

    node->UpdateTransformHierarchy();

    EXPECT_MAT4_EQ(node->GetCachedWorldTransform(), {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
}

TEST(Node, UpdateTransformsWithParent) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->transform.SetScale(2.0f);
    parent->UpdateTransformHierarchy();

    EXPECT_MAT4_EQ(child->GetCachedWorldTransform(), {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
}

TEST(Node, UpdateTransformsComposeParentFirst) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->transform.SetPosition({5.0f, 0.0f, 0.0f});
    child->transform.SetPosition({1.0f, 2.0f, 3.0f});
    parent->UpdateTransformHierarchy();

    EXPECT_MAT4_EQ(child->GetCachedWorldTransform(), {
        1.0f, 0.0f, 0.0f, 6.0f,
        0.0f, 1.0f, 0.0f, 2.0f,
        0.0f, 0.0f, 1.0f, 3.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
}

TEST(Node, UpdateTransformsPropagateToCleanChildren) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->UpdateTransformHierarchy();
    parent->transform.SetPosition({5.0f, 0.0f, 0.0f});
    parent->UpdateTransformHierarchy();

    EXPECT_MAT4_EQ(child->GetCachedWorldTransform(), {
        1.0f, 0.0f, 0.0f, 5.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
}

TEST(Node, DisableTransformAutoUpdate) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());
    child->transform_auto_update = false;

    parent->transform.SetScale(2.0f);
    parent->UpdateTransformHierarchy();

    EXPECT_MAT4_EQ(parent->GetWorldTransform(), {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });

    EXPECT_MAT4_EQ(child->GetWorldTransform(), {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
}

TEST(Node, DisableTransformAutoUpdateIgnoresDirtyParent) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());
    child->transform_auto_update = false;

    parent->UpdateTransformHierarchy();
    parent->transform.SetPosition({5.0f, 0.0f, 0.0f});

    // A frozen node reports its cache even though an ancestor is dirty.
    EXPECT_MAT4_EQ(child->GetWorldTransform(), {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
}

TEST(Node, MarkTransformedNodeAsTouched) {
    auto node = vglx::Node::Create();
    node->UpdateTransformHierarchy();

    node->transform.SetScale(0.5f);

    EXPECT_TRUE(node->transform.touched);
}

TEST(Node, MarkTransformedNodeAsUntouched) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->UpdateTransformHierarchy();

    EXPECT_FALSE(parent->transform.touched);
    EXPECT_FALSE(child->transform.touched);
}

TEST(Node, MarkDetachedNodesAsTouched) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->UpdateTransformHierarchy();

    auto detached = parent->Detach(child);

    EXPECT_TRUE(detached->transform.touched);
}

#pragma endregion

#pragma region World Transform Queries

TEST(Node, WorldTransformWithDirtyAncestor) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->UpdateTransformHierarchy();
    parent->transform.SetPosition({5.0f, 0.0f, 0.0f});

    // The query is current without a hierarchy update...
    EXPECT_MAT4_EQ(child->GetWorldTransform(), {
        1.0f, 0.0f, 0.0f, 5.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });

    // ...while the cache still reflects the last update.
    EXPECT_MAT4_EQ(child->GetCachedWorldTransform(), {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
}

TEST(Node, WorldTransformQueryPreservesDirtyState) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());
    auto grandchild = child->Add(vglx::Node::Create());

    parent->UpdateTransformHierarchy();
    parent->transform.SetPosition({3.0f, 0.0f, 0.0f});

    (void)parent->GetWorldTransform();
    EXPECT_TRUE(parent->transform.touched);

    parent->UpdateTransformHierarchy();

    EXPECT_MAT4_EQ(grandchild->GetCachedWorldTransform(), {
        1.0f, 0.0f, 0.0f, 3.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
    EXPECT_FALSE(parent->transform.touched);
    EXPECT_FALSE(child->transform.touched);
    EXPECT_FALSE(grandchild->transform.touched);
}

TEST(Node, WorldPositionWithParent) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->transform.SetPosition({5.0f, 0.0f, 0.0f});
    child->transform.SetPosition({1.0f, 2.0f, 3.0f});

    EXPECT_VEC3_EQ(child->GetWorldPosition(), {6.0f, 2.0f, 3.0f});
}

TEST(Node, WorldPositionWithRotatedParent) {
    auto parent = vglx::Node::Create();
    auto child = parent->Add(vglx::Node::Create());

    parent->transform.Rotate(vglx::Vector3::UnitZ(), vglx::math::pi_over_2);
    child->transform.SetPosition({1.0f, 0.0f, 0.0f});

    EXPECT_VEC3_NEAR(child->GetWorldPosition(), {0.0f, 1.0f, 0.0f}, 0.001f);
}

#pragma endregion

#pragma region Scene Attachment

TEST(Node, GetSceneWithoutScene) {
    auto node = vglx::Node::Create();
    EXPECT_EQ(node->GetScene(), nullptr);
}

TEST(Node, GetSceneAfterAttach) {
    auto scene = vglx::Scene::Create();
    auto child = scene->Add(vglx::Node::Create());
    auto grandchild = child->Add(vglx::Node::Create());

    EXPECT_EQ(child->GetScene(), scene.get());
    EXPECT_EQ(grandchild->GetScene(), scene.get());
}

TEST(Node, GetSceneAfterDetach) {
    auto scene = vglx::Scene::Create();
    auto child = scene->Add(vglx::Node::Create());
    auto grandchild = child->Add(vglx::Node::Create());

    auto detached = scene->Detach(child);

    EXPECT_EQ(detached->GetScene(), nullptr);
    EXPECT_EQ(grandchild->GetScene(), nullptr);
}

#pragma endregion

#pragma region Assertions

TEST(Node, DeathWhenAddingNullptr) {
#ifdef NDEBUG
    GTEST_SKIP() << "VGLX_ASSERT is disabled in release builds";
#endif

    auto parent = vglx::Node::Create();

    EXPECT_DEATH({
        parent->Add(std::unique_ptr<vglx::Node> {});
    }, ".*nullptr");
}

TEST(Node, DeathWhenAddingAncestor) {
#ifdef NDEBUG
    GTEST_SKIP() << "VGLX_ASSERT is disabled in release builds";
#endif

    auto root = vglx::Node::Create();
    auto parent = root->Add(vglx::Node::Create());
    auto child = parent->Add(vglx::Node::Create());

    auto detached = root->Detach(parent);

    EXPECT_DEATH({
        child->Add(std::move(detached));
    }, ".*ancestor");
}

TEST(Node, DeathWhenDetachingNonChild) {
#ifdef NDEBUG
    GTEST_SKIP() << "VGLX_ASSERT is disabled in release builds";
#endif

    auto parent = vglx::Node::Create();
    auto child = vglx::Node::Create();

    EXPECT_DEATH({
        parent->Remove(child.get());
    }, ".*not a child");
}

#pragma endregion
