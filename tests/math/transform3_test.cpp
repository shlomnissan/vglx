/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/math/quaternion.hpp>
#include <vglx/math/transform3.hpp>
#include <vglx/math/utilities.hpp>

#pragma region Mutators

TEST(Transform3, SetPosition) {
    auto t1 = vglx::Transform3 {};
    t1.SetPosition({2.0f, 1.0f, 3.0f});

    EXPECT_VEC3_EQ(t1.position, {2.0f, 1.0f, 3.0f});
    EXPECT_MAT4_EQ(t1.Get(), {
        1.0f, 0.0f, 0.0f, 2.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 3.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });

    constexpr auto t2 = []() {
        auto t = vglx::Transform3 {};
        t.SetPosition({2.0f, 1.0f, 3.0f});
        return t;
    }();

    static_assert(t2.position.x == 2.0f);
    static_assert(t2.position.y == 1.0f);
    static_assert(t2.position.z == 3.0f);
}

TEST(Transform3, SetScale) {
    auto t1 = vglx::Transform3 {};
    t1.SetScale({2.0f, 1.0f, 3.0f});

    EXPECT_VEC3_EQ(t1.scale, {2.0f, 1.0f, 3.0f});
    EXPECT_MAT4_EQ(t1.Get(), {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 3.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });

    constexpr auto t2 = []() {
        auto t = vglx::Transform3 {};
        t.SetScale({2.0f, 1.0f, 3.0f});
        return t;
    }();

    static_assert(t2.scale.x == 2.0f);
    static_assert(t2.scale.y == 1.0f);
    static_assert(t2.scale.z == 3.0f);
}

TEST(Transform3, SetRotationFromEuler) {
    constexpr auto e = vglx::Euler {0.1f, 0.2f, 0.3f};

    auto t1 = vglx::Transform3 {};
    t1.SetRotation(e);

    // The rotation is stored as a quaternion; Get() reproduces the Euler matrix.
    EXPECT_MAT4_NEAR(t1.Get(), e.GetMatrix(), 1e-4);

    // Euler read-back is well defined away from gimbal lock.
    const auto out = t1.GetEuler();
    EXPECT_NEAR(out.pitch, e.pitch, 1e-4);
    EXPECT_NEAR(out.yaw, e.yaw, 1e-4);
    EXPECT_NEAR(out.roll, e.roll, 1e-4);

    constexpr auto t2 = []() {
        auto t = vglx::Transform3 {};
        t.SetRotation(vglx::Euler {0.1f, 0.2f, 0.3f});
        return t;
    }();

    static_assert(ApproxEqual(t2.GetEuler().pitch, 0.1f));
}

TEST(Transform3, SetRotationFromQuaternion) {
    constexpr auto q = vglx::Quaternion::FromAxisAngle(vglx::Vector3::UnitY(), vglx::math::pi_over_2);

    auto t1 = vglx::Transform3 {};
    t1.SetRotation(q);

    EXPECT_EQ(t1.rotation, q);
    EXPECT_MAT4_NEAR(t1.Get(), q.GetMatrix(), 1e-4);

    constexpr auto t2 = []() {
        auto t = vglx::Transform3 {};
        t.SetRotation(vglx::Quaternion::FromAxisAngle(vglx::Vector3::UnitY(), vglx::math::pi_over_2));
        return t;
    }();

    static_assert(t2.rotation == q);
}

TEST(Transform3, MultipleTransformations) {
    constexpr auto rotation = vglx::Euler {
        vglx::math::pi_over_2 + 0.1f,
        vglx::math::pi_over_2 + 0.2f,
        vglx::math::pi_over_2 + 0.3f
    };
    constexpr auto position = vglx::Vector3 {2.0f, 1.0f, 3.0f};
    constexpr auto scale = vglx::Vector3 {2.0f, 1.0f, 3.0f};

    auto t = vglx::Transform3 {};
    t.SetPosition(position);
    t.SetScale(scale);
    t.SetRotation(rotation);

    const auto cos_p = vglx::math::Cos(rotation.pitch);
    const auto sin_p = vglx::math::Sin(rotation.pitch);
    const auto cos_y = vglx::math::Cos(rotation.yaw);
    const auto sin_y = vglx::math::Sin(rotation.yaw);
    const auto cos_r = vglx::math::Cos(rotation.roll);
    const auto sin_r = vglx::math::Sin(rotation.roll);

    EXPECT_MAT4_NEAR(t.Get(), {
        scale.x * (cos_y * cos_r + sin_y * sin_p * sin_r),
        scale.y * (sin_y * sin_p * cos_r - cos_y * sin_r),
        scale.z * (sin_y * cos_p),
        position.x,

        scale.x * (cos_p * sin_r),
        scale.y * (cos_p * cos_r),
        scale.z * (-sin_p),
        position.y,

        scale.x * (cos_y * sin_p * sin_r - sin_y * cos_r),
        scale.y * (sin_y * sin_r + cos_y * sin_p * cos_r),
        scale.z * (cos_y * cos_p),
        position.z,

        0.0f, 0.0f, 0.0f, 1.0f
    }, 1e-4);
}

#pragma endregion

#pragma region Cumulative Transformations

TEST(Transform3, Translate) {
    auto t1 = vglx::Transform3 {};
    t1.Translate({2.0f, 1.0f, 3.0f});
    t1.Translate({1.0f, 1.0f, 0.0f});

    EXPECT_VEC3_EQ(t1.position, {3.0f, 2.0f, 3.0f});
    EXPECT_MAT4_EQ(t1.Get(), {
        1.0f, 0.0f, 0.0f, 3.0f,
        0.0f, 1.0f, 0.0f, 2.0f,
        0.0f, 0.0f, 1.0f, 3.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });

    constexpr auto t2 = []() {
        auto t = vglx::Transform3 {};
        t.Translate({2.0f, 1.0f, 3.0f});
        t.Translate({1.0f, 1.0f, 0.0f});
        return t;
    }();

    static_assert(t2.position.x == 3.0f);
    static_assert(t2.position.y == 2.0f);
    static_assert(t2.position.z == 3.0f);
}

TEST(Transform3, Scale) {
    auto t1 = vglx::Transform3 {};
    t1.Scale({2.0f, 2.0f, 2.0f});
    t1.Scale({3.0f, 3.0f, 2.0f});

    EXPECT_VEC3_EQ(t1.scale, {6.0f, 6.0f, 4.0f});
    EXPECT_MAT4_EQ(t1.Get(), {
        6.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 6.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 4.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });

    constexpr auto t2 = []() {
        auto t = vglx::Transform3 {};
            t.Scale({2.0f, 2.0f, 2.0f});
            t.Scale({3.0f, 3.0f, 2.0f});
        return t;
    }();

    static_assert(t2.scale.x == 6.0f);
    static_assert(t2.scale.y == 6.0f);
    static_assert(t2.scale.z == 4.0f);
}

TEST(Transform3, RotateX) {
    auto t = vglx::Transform3 {};
    t.Rotate(vglx::Vector3::UnitX(), vglx::math::pi_over_2);
    t.Rotate(vglx::Vector3::UnitX(), 0.1f);

    constexpr auto c = vglx::math::Cos(vglx::math::pi_over_2 + 0.1f);
    constexpr auto s = vglx::math::Sin(vglx::math::pi_over_2 + 0.1f);

    EXPECT_MAT4_NEAR(t.Get(), {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, -s, 0.0f,
        0.0f, s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }, 1e-4);

    constexpr auto m = []() {
        auto t = vglx::Transform3 {};
        t.Rotate(vglx::Vector3::UnitX(), vglx::math::pi_over_2);
        t.Rotate(vglx::Vector3::UnitX(), 0.1f);
        return t.Get();
    }();

    static_assert(ApproxEqual(m[1].y, c));
    static_assert(ApproxEqual(m[1].z, s));
    static_assert(ApproxEqual(m[2].y, -s));
    static_assert(ApproxEqual(m[2].z, c));
}

TEST(Transform3, RotateY) {
    auto t = vglx::Transform3 {};
    t.Rotate(vglx::Vector3::UnitY(), vglx::math::pi_over_2);
    t.Rotate(vglx::Vector3::UnitY(), 0.1f);

    constexpr auto c = vglx::math::Cos(vglx::math::pi_over_2 + 0.1f);
    constexpr auto s = vglx::math::Sin(vglx::math::pi_over_2 + 0.1f);

    EXPECT_MAT4_NEAR(t.Get(), {
        c, 0.0f, s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }, 1e-4);

    constexpr auto m = []() {
        auto t = vglx::Transform3 {};
        t.Rotate(vglx::Vector3::UnitY(), vglx::math::pi_over_2);
        t.Rotate(vglx::Vector3::UnitY(), 0.1f);
        return t.Get();
    }();

    static_assert(ApproxEqual(m[0].x, c));
    static_assert(ApproxEqual(m[0].z, -s));
    static_assert(ApproxEqual(m[2].x, s));
    static_assert(ApproxEqual(m[2].z, c));
}

TEST(Transform3, RotateZ) {
    auto t = vglx::Transform3 {};
    t.Rotate(vglx::Vector3::UnitZ(), vglx::math::pi_over_2);
    t.Rotate(vglx::Vector3::UnitZ(), 0.1f);

    constexpr auto c = vglx::math::Cos(vglx::math::pi_over_2 + 0.1f);
    constexpr auto s = vglx::math::Sin(vglx::math::pi_over_2 + 0.1f);

    EXPECT_MAT4_NEAR(t.Get(), {
        c, -s, 0.0f, 0.0f,
        s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }, 1e-4);

    constexpr auto m = []() {
        auto t = vglx::Transform3 {};
        t.Rotate(vglx::Vector3::UnitZ(), vglx::math::pi_over_2);
        t.Rotate(vglx::Vector3::UnitZ(), 0.1f);
        return t.Get();
    }();

    static_assert(ApproxEqual(m[0].x, c));
    static_assert(ApproxEqual(m[0].y, s));
    static_assert(ApproxEqual(m[1].x, -s));
    static_assert(ApproxEqual(m[1].y, c));
}

TEST(Transform3, RotateArbitraryAxis) {
    constexpr auto axis = vglx::Vector3 {1.0f, 2.0f, 3.0f};
    constexpr auto angle = 1.0f;

    auto t = vglx::Transform3 {};
    t.Rotate(axis, angle);

    EXPECT_MAT4_NEAR(t.Get(), vglx::Quaternion::FromAxisAngle(axis, angle).GetMatrix(), 1e-4);

    constexpr auto m = []() {
        auto t = vglx::Transform3 {};
        t.Rotate(vglx::Vector3 {1.0f, 2.0f, 3.0f}, 1.0f);
        return t.Get();
    }();

    constexpr auto expected = vglx::Quaternion::FromAxisAngle(axis, angle).GetMatrix();
    static_assert(ApproxEqual(m[0].x, expected[0].x));
    static_assert(ApproxEqual(m[1].y, expected[1].y));
    static_assert(ApproxEqual(m[2].z, expected[2].z));
}

#pragma endregion

#pragma region Local-Space Translation

TEST(Transform3, TranslateBeforeRotation) {
    auto t = vglx::Transform3 {};
    t.Translate({0.0f, 0.0f, 1.0f});
    t.Rotate(vglx::Vector3::UnitY(), vglx::math::pi_over_2);

    EXPECT_MAT4_NEAR(t.Get(), {
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 1.0f,
         0.0f, 0.0f, 0.0f, 1.0f
    }, 1e-4);

    constexpr auto m = []() {
        auto t = vglx::Transform3 {};
        t.Translate({0.0f, 0.0f, 1.0f});
        t.Rotate(vglx::Vector3::UnitY(), vglx::math::pi_over_2);
        return t.Get();
    }();

    static_assert(ApproxEqual(m[0].x, 0.0f));
    static_assert(ApproxEqual(m[0].y, 0.0f));
    static_assert(ApproxEqual(m[0].z, -1.0f));
    static_assert(ApproxEqual(m[1].x, 0.0f));
    static_assert(ApproxEqual(m[1].y, 1.0f));
    static_assert(ApproxEqual(m[1].z, 0.0f));
    static_assert(ApproxEqual(m[2].x, 1.0f));
    static_assert(ApproxEqual(m[2].y, 0.0f));
    static_assert(ApproxEqual(m[2].z, 0.0f));
    static_assert(ApproxEqual(m[3].x, 0.0f));
    static_assert(ApproxEqual(m[3].y, 0.0f));
    static_assert(ApproxEqual(m[3].z, 1.0f));
}

TEST(Transform3, TranslateAfterRotation) {
    auto t = vglx::Transform3 {};
    t.Rotate(vglx::Vector3::UnitY(), vglx::math::pi_over_2);
    t.Translate({0.0f, 0.0f, 1.0f});

    EXPECT_MAT4_NEAR(t.Get(), {
         0.0f, 0.0f, 1.0f, 1.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f
    }, 1e-4);

    constexpr auto m = []() {
        auto t = vglx::Transform3 {};
        t.Rotate(vglx::Vector3::UnitY(), vglx::math::pi_over_2);
        t.Translate({0.0f, 0.0f, 1.0f});
        return t.Get();
    }();

    static_assert(ApproxEqual(m[0].x, 0.0f));
    static_assert(ApproxEqual(m[0].y, 0.0f));
    static_assert(ApproxEqual(m[0].z, -1.0f));
    static_assert(ApproxEqual(m[1].x, 0.0f));
    static_assert(ApproxEqual(m[1].y, 1.0f));
    static_assert(ApproxEqual(m[1].z, 0.0f));
    static_assert(ApproxEqual(m[2].x, 1.0f));
    static_assert(ApproxEqual(m[2].y, 0.0f));
    static_assert(ApproxEqual(m[2].z, 0.0f));
    static_assert(ApproxEqual(m[3].x, 1.0f));
    static_assert(ApproxEqual(m[3].y, 0.0f));
    static_assert(ApproxEqual(m[3].z, 0.0f));
}

#pragma endregion

#pragma region Decompose

TEST(Transform3, DecomposeRoundTrip) {
    auto source = vglx::Transform3 {};
    source.SetPosition({2.0f, 1.0f, 3.0f});
    source.SetRotation(vglx::Euler {0.1f, 0.2f, 0.3f});
    source.SetScale({2.0f, 3.0f, 4.0f});

    auto t1 = vglx::Transform3 {source.Get()};

    EXPECT_VEC3_NEAR(t1.position, {2.0f, 1.0f, 3.0f}, 1e-4);
    EXPECT_VEC3_NEAR(t1.scale, {2.0f, 3.0f, 4.0f}, 1e-4);
    EXPECT_MAT4_NEAR(t1.Get(), source.Get(), 1e-4);

    // SetFromMatrix replaces every component, not just the ones it can recover.
    auto t2 = vglx::Transform3 {};
    t2.SetPosition({-9.0f, -9.0f, -9.0f});
    t2.SetFromMatrix(source.Get());

    EXPECT_VEC3_NEAR(t2.position, {2.0f, 1.0f, 3.0f}, 1e-4);
    EXPECT_MAT4_NEAR(t2.Get(), source.Get(), 1e-4);

    constexpr auto t3 = []() {
        auto t = vglx::Transform3 {};
        t.SetPosition({2.0f, 1.0f, 3.0f});
        t.SetScale({2.0f, 3.0f, 4.0f});
        return vglx::Transform3 {t.Get()};
    }();

    static_assert(ApproxEqual(t3.position.x, 2.0f));
    static_assert(ApproxEqual(t3.position.y, 1.0f));
    static_assert(ApproxEqual(t3.position.z, 3.0f));
    static_assert(ApproxEqual(t3.scale.x, 2.0f));
    static_assert(ApproxEqual(t3.scale.y, 3.0f));
    static_assert(ApproxEqual(t3.scale.z, 4.0f));
}

TEST(Transform3, DecomposeMirroredBasis) {
    auto source = vglx::Transform3 {};
    source.SetRotation(vglx::Euler {0.1f, 0.2f, 0.3f});
    source.SetScale({1.0f, 1.0f, -1.0f});

    auto t = vglx::Transform3 {};
    t.SetFromMatrix(source.Get());

    // The sign of the mirror is attributed to x regardless of which axis
    // carried it originally, so the components differ from the source while
    // the composed matrix does not.
    EXPECT_VEC3_NEAR(t.scale, {-1.0f, 1.0f, 1.0f}, 1e-4);
    EXPECT_MAT4_NEAR(t.Get(), source.Get(), 1e-4);
}

TEST(Transform3, DecomposeDegenerateBasis) {
    // The y axis has zero length, so the basis encodes no recoverable rotation.
    constexpr auto mat = vglx::Matrix4 {
        1.0f, 0.0f, 0.0f, 4.0f,
        0.0f, 0.0f, 0.0f, 5.0f,
        0.0f, 0.0f, 1.0f, 6.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    auto t = vglx::Transform3 {mat};

    EXPECT_VEC3_NEAR(t.position, {4.0f, 5.0f, 6.0f}, 1e-4);
    EXPECT_VEC3_NEAR(t.scale, {1.0f, 0.0f, 1.0f}, 1e-4);
    EXPECT_EQ(t.rotation, vglx::Quaternion {});
}

#pragma endregion

#pragma region Const Access

TEST(Transform3, ConstGetPreservesTouched) {
    auto t1 = vglx::Transform3 {};
    t1.SetPosition({2.0f, 1.0f, 3.0f});

    const auto& c1 = t1;

    EXPECT_MAT4_EQ(c1.Get(), {
        1.0f, 0.0f, 0.0f, 2.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 3.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    });
    EXPECT_TRUE(t1.touched);

    constexpr auto t2 = []() {
        auto t = vglx::Transform3 {};
        t.SetPosition({2.0f, 1.0f, 3.0f});
        const auto& c = t;
        (void)c.Get();
        return t;
    }();

    static_assert(t2.touched);
}

#pragma endregion
