/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/core/disposable.hpp>
#include <vglx/geometries/buffer_attribute.hpp>
#include <vglx/geometries/geometry.hpp>
#include <vglx/math/box3.hpp>
#include <vglx/math/matrix4.hpp>

#include <string>
#include <utility>
#include <vector>

namespace {

const auto kTrianglePositions = std::vector<float> {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

auto create_positions(std::vector<float> data) {
    return vglx::BufferAttribute::Create({
        .name = vglx::BufferAttribute::kPosition,
        .format = vglx::BufferAttribute::Format::Float32x3,
        .rate = vglx::BufferAttribute::Rate::Vertex
    }, std::move(data));
}

auto create_tex_coords(std::vector<float> data) {
    return vglx::BufferAttribute::Create({
        .name = vglx::BufferAttribute::kTexCoord,
        .format = vglx::BufferAttribute::Format::Float32x2,
        .rate = vglx::BufferAttribute::Rate::Vertex
    }, std::move(data));
}

auto create_normals(std::vector<float> data) {
    return vglx::BufferAttribute::Create({
        .name = vglx::BufferAttribute::kNormal,
        .format = vglx::BufferAttribute::Format::Float32x3,
        .rate = vglx::BufferAttribute::Rate::Vertex
    }, std::move(data));
}

auto create_tangents(std::vector<float> data) {
    return vglx::BufferAttribute::Create({
        .name = vglx::BufferAttribute::kTangent,
        .format = vglx::BufferAttribute::Format::Float32x4,
        .rate = vglx::BufferAttribute::Rate::Vertex
    }, std::move(data));
}

auto create_translation(float x, float y, float z) {
    return vglx::Matrix4 {
        1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

auto create_scale(float x, float y, float z) {
    return vglx::Matrix4 {
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

}

#pragma region Attributes

TEST(Geometry, AddAttribute) {
    auto geometry = vglx::Geometry::Create();
    auto positions = create_positions(kTrianglePositions);

    geometry->AddAttribute(positions);

    EXPECT_EQ(geometry->GetAttribute(vglx::BufferAttribute::kPosition), positions);
    EXPECT_EQ(geometry->VertexCount(), 3);
    EXPECT_EQ(geometry->GetAttributes().size(), 1);
}

TEST(Geometry, AddMultipleAttributesWithMatchingCounts) {
    auto geometry = vglx::Geometry::Create();

    geometry->AddAttribute(create_positions(kTrianglePositions));
    geometry->AddAttribute(create_tex_coords({
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.5f, 1.0f
    }));

    EXPECT_NE(geometry->GetAttribute(vglx::BufferAttribute::kPosition), nullptr);
    EXPECT_NE(geometry->GetAttribute(vglx::BufferAttribute::kTexCoord), nullptr);
    EXPECT_EQ(geometry->VertexCount(), 3);
    EXPECT_EQ(geometry->GetAttributes().size(), 2);
}

TEST(Geometry, RejectsAttributeWithDuplicateName) {
    auto geometry = vglx::Geometry::Create();
    auto first_attribute = create_positions(kTrianglePositions);

    geometry->AddAttribute(first_attribute);
    geometry->AddAttribute(create_positions(kTrianglePositions));

    EXPECT_EQ(geometry->GetAttribute(vglx::BufferAttribute::kPosition), first_attribute);
    EXPECT_EQ(geometry->GetAttributes().size(), 1);
}

TEST(Geometry, RejectsAttributeWithInstanceRate) {
    auto geometry = vglx::Geometry::Create();

    geometry->AddAttribute(vglx::BufferAttribute::Create({
        .name = vglx::BufferAttribute::kInstanceColor,
        .format = vglx::BufferAttribute::Format::Float32x3,
        .rate = vglx::BufferAttribute::Rate::Instance
    }, {1.0f, 0.0f, 0.0f}));

    EXPECT_EQ(geometry->GetAttribute(vglx::BufferAttribute::kInstanceColor), nullptr);
    EXPECT_EQ(geometry->GetAttributes().size(), 0);
}

TEST(Geometry, RejectsInvalidAttribute) {
    auto geometry = vglx::Geometry::Create();

    geometry->AddAttribute(create_positions({}));

    EXPECT_EQ(geometry->GetAttribute(vglx::BufferAttribute::kPosition), nullptr);
    EXPECT_EQ(geometry->GetAttributes().size(), 0);
}

TEST(Geometry, RejectsAttributeWithElementCountMismatch) {
    auto geometry = vglx::Geometry::Create();

    geometry->AddAttribute(create_positions(kTrianglePositions));

    // 4 elements for 3 existing vertices
    geometry->AddAttribute(create_tex_coords({
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    }));

    EXPECT_EQ(geometry->GetAttribute(vglx::BufferAttribute::kTexCoord), nullptr);
    EXPECT_EQ(geometry->GetAttributes().size(), 1);
}

#pragma endregion

#pragma region Indices

TEST(Geometry, SetIndices) {
    auto geometry = vglx::Geometry::Create();

    geometry->SetIndices({0, 1, 2, 2, 3, 0});

    EXPECT_EQ(geometry->GetIndexData().size(), 6);
    EXPECT_EQ(geometry->GetMaxIndex(), 3);
    EXPECT_EQ(geometry->GetIndexVersion(), 1);
}

TEST(Geometry, SetIndicesWithEmptyList) {
    auto geometry = vglx::Geometry::Create();

    geometry->SetIndices({0, 1, 2});
    geometry->SetIndices({});

    EXPECT_TRUE(geometry->GetIndexData().empty());
    EXPECT_EQ(geometry->GetMaxIndex(), 0);
    EXPECT_EQ(geometry->GetIndexVersion(), 2);
}

#pragma endregion

#pragma region Bounding Volumes

TEST(Geometry, BoundingBox) {
    auto geometry = vglx::Geometry::Create();
    geometry->AddAttribute(create_positions(kTrianglePositions));

    const auto box = geometry->BoundingBox();

    EXPECT_VEC3_EQ(box.min, {-0.5f, -0.5f, 0.0f});
    EXPECT_VEC3_EQ(box.max, {0.5f, 0.5f, 0.0f});
}

TEST(Geometry, BoundingSphere) {
    auto geometry = vglx::Geometry::Create();
    geometry->AddAttribute(create_positions({
        -1.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f
    }));

    const auto sphere = geometry->BoundingSphere();

    EXPECT_VEC3_EQ(sphere.center, {0.0f, 0.0f, 0.0f});
    EXPECT_NEAR(sphere.radius, 1.0f, 1e-4f);
}

TEST(Geometry, BoundingBoxWithoutPositionAttribute) {
    auto geometry = vglx::Geometry::Create();

    const auto box = geometry->BoundingBox();
    const auto default_box = vglx::Box3 {};

    EXPECT_VEC3_EQ(box.min, default_box.min);
    EXPECT_VEC3_EQ(box.max, default_box.max);
}

TEST(Geometry, BoundingSphereWithoutPositionAttribute) {
    auto geometry = vglx::Geometry::Create();

    const auto sphere = geometry->BoundingSphere();

    EXPECT_FLOAT_EQ(sphere.radius, -1.0f);
}

TEST(Geometry, BoundingBoxInvalidatedWhenPositionDataChanges) {
    auto geometry = vglx::Geometry::Create();
    auto positions = create_positions(kTrianglePositions);
    geometry->AddAttribute(positions);

    const auto before = geometry->BoundingBox();
    EXPECT_VEC3_EQ(before.max, {0.5f, 0.5f, 0.0f});

    positions->SetData({
        -2.0f, -2.0f, 0.0f,
         2.0f, -2.0f, 0.0f,
         0.0f,  2.0f, 0.0f
    });

    const auto after = geometry->BoundingBox();
    EXPECT_VEC3_EQ(after.min, {-2.0f, -2.0f, 0.0f});
    EXPECT_VEC3_EQ(after.max, {2.0f, 2.0f, 0.0f});
}

TEST(Geometry, BoundingSphereInvalidatedWhenPositionDataChanges) {
    auto geometry = vglx::Geometry::Create();
    auto positions = create_positions({
        -1.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f
    });
    geometry->AddAttribute(positions);

    const auto before = geometry->BoundingSphere();
    EXPECT_NEAR(before.radius, 1.0f, 1e-4f);

    positions->SetData({
        -3.0f, 0.0f, 0.0f,
         3.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f
    });

    const auto after = geometry->BoundingSphere();
    EXPECT_NEAR(after.radius, 3.0f, 1e-4f);
}

#pragma endregion

#pragma region Transform

TEST(Geometry, ApplyTransformTranslatesPositions) {
    auto geometry = vglx::Geometry::Create();
    auto positions = create_positions(kTrianglePositions);
    auto tex_coords = create_tex_coords({0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f});

    geometry->AddAttribute(positions);
    geometry->AddAttribute(tex_coords);
    geometry->ApplyTransform(create_translation(1.0f, 2.0f, 3.0f));

    const auto box = geometry->BoundingBox();

    EXPECT_VEC3_EQ(box.min, {0.5f, 1.5f, 3.0f});
    EXPECT_VEC3_EQ(box.max, {1.5f, 2.5f, 3.0f});
    EXPECT_EQ(positions->GetVersion(), 1);
    EXPECT_EQ(tex_coords->GetVersion(), 0);
}

TEST(Geometry, ApplyTransformScalesNormalsWithInverseTranspose) {
    auto geometry = vglx::Geometry::Create();
    auto normals = create_normals({0.7071f, 0.7071f, 0.0f});

    geometry->AddAttribute(create_positions({0.0f, 0.0f, 0.0f}));
    geometry->AddAttribute(normals);
    geometry->ApplyTransform(create_scale(2.0f, 1.0f, 1.0f));

    const auto& data = normals->GetData();

    EXPECT_VEC3_NEAR({data[0], data[1], data[2]}, {0.4472f, 0.8944f, 0.0f}, 1e-4f);
}

TEST(Geometry, ApplyTransformFlipsTangentHandednessWhenMirrored) {
    auto geometry = vglx::Geometry::Create();
    auto tangents = create_tangents({1.0f, 0.0f, 0.0f, 1.0f});

    geometry->AddAttribute(create_positions({0.0f, 0.0f, 0.0f}));
    geometry->AddAttribute(tangents);
    geometry->ApplyTransform(create_scale(-1.0f, 1.0f, 1.0f));

    const auto& data = tangents->GetData();

    EXPECT_VEC4_NEAR({data[0], data[1], data[2], data[3]}, {-1.0f, 0.0f, 0.0f, -1.0f}, 1e-4f);
}

#pragma endregion

#pragma region Disposal

TEST(Geometry, DisposeFiresCallbackOnce) {
    auto geometry = vglx::Geometry::Create();
    auto calls = 0;

    geometry->OnDispose([&calls](const std::string&) { calls++; });
    geometry->Dispose();
    geometry->Dispose();

    EXPECT_EQ(calls, 1);
}

TEST(Geometry, DisposeFiresOnDestruction) {
    auto calls = 0;

    {
        auto geometry = vglx::Geometry::Create();
        geometry->OnDispose([&calls](const std::string&) { calls++; });
    }

    EXPECT_EQ(calls, 1);
}

#pragma endregion
