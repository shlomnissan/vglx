/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/canvas/sprite.hpp>
#include <vglx/textures/image.hpp>
#include <vglx/textures/texture_2d.hpp>

namespace {

auto make_texture(unsigned width, unsigned height) {
    return vglx::Texture2D::Create(vglx::Image::Create({
        .width = width,
        .height = height
    }));
}

}

#pragma region Size

TEST(Sprite, SizeMatchesTexture) {
    auto sprite = vglx::Sprite::Create(make_texture(64, 32));

    EXPECT_VEC2_EQ(sprite->GetSize(), {64.0f, 32.0f});
}

TEST(Sprite, SizeMatchesRegion) {
    auto sprite = vglx::Sprite::Create(make_texture(64, 32));
    sprite->region = vglx::Rect {16.0f, 0.0f, 16.0f, 8.0f};

    EXPECT_VEC2_EQ(sprite->GetSize(), {16.0f, 8.0f});
}

TEST(Sprite, SizeIsZeroWithoutTexture) {
    auto sprite = vglx::Sprite::Create(nullptr);

    EXPECT_VEC2_EQ(sprite->GetSize(), {0.0f, 0.0f});
}

#pragma endregion

#pragma region Geometry

TEST(Sprite, GeometryTransformAppliesSizeAndAnchor) {
    auto sprite = vglx::Sprite::Create(make_texture(64, 32));
    sprite->region = vglx::Rect {16.0f, 0.0f, 16.0f, 8.0f};
    sprite->anchor = {0.5f, 0.5f};

    const auto transform = sprite->GetGeometryTransform();

    EXPECT_VEC3_EQ(transform * vglx::Vector3 {0.0f, 0.0f, 1.0f}, {-8.0f, -4.0f, 1.0f});
    EXPECT_VEC3_EQ(transform * vglx::Vector3 {1.0f, 1.0f, 1.0f}, {8.0f, 4.0f, 1.0f});
}

TEST(Sprite, TextureTransformMapsRegionWithFlippedV) {
    auto sprite = vglx::Sprite::Create(make_texture(64, 32));
    sprite->region = vglx::Rect {16.0f, 0.0f, 16.0f, 8.0f};

    const auto transform = sprite->GetTextureTransform();

    EXPECT_VEC3_EQ(transform * vglx::Vector3 {0.0f, 0.0f, 1.0f}, {0.25f, 1.0f, 1.0f});
    EXPECT_VEC3_EQ(transform * vglx::Vector3 {1.0f, 1.0f, 1.0f}, {0.5f, 0.75f, 1.0f});
}

TEST(Sprite, TransformsAreIdentityWithoutTexture) {
    auto sprite = vglx::Sprite::Create(nullptr);

    EXPECT_MAT3_EQ(sprite->GetTextureTransform(), vglx::Matrix3 {1.0f});
    EXPECT_VEC3_EQ(sprite->GetGeometryTransform() * vglx::Vector3 {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f});
}

#pragma endregion

#pragma region Node Identity

TEST(Sprite, IsRenderableSpriteNode) {
    auto sprite = vglx::Sprite::Create(nullptr);

    EXPECT_EQ(sprite->GetNodeType(), vglx::Node2D::Type::Sprite);
    EXPECT_TRUE(sprite->IsRenderable());
}

#pragma endregion
