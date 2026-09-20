/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/canvas/font.hpp>

#pragma region Glyphs

TEST(Font, GlyphsAreRetrievableRegardlessOfInsertionOrder) {
    auto font = vglx::Font::Create();

    for (auto code : {U'm', U'a', U'z', U'b'}) {
        font->AddGlyph(code, {.advance = static_cast<float>(code)});
    }

    for (auto code : {U'a', U'b', U'm', U'z'}) {
        auto glyph = font->GetGlyph(code);

        ASSERT_TRUE(glyph.has_value());
        EXPECT_FLOAT_EQ(glyph->advance, static_cast<float>(code));
    }
}

TEST(Font, AddGlyphOverwritesExistingEntry) {
    auto font = vglx::Font::Create();

    font->AddGlyph(U'a', {.offset = {1.0f, 2.0f}, .advance = 4.0f});
    font->AddGlyph(U'a', {.offset = {3.0f, 4.0f}, .advance = 9.0f});

    auto glyph = font->GetGlyph(U'a');

    ASSERT_TRUE(glyph.has_value());
    EXPECT_FLOAT_EQ(glyph->advance, 9.0f);
    EXPECT_VEC2_EQ(glyph->offset, {3.0f, 4.0f});
}

TEST(Font, GetGlyphReturnsNulloptForMissingCode) {
    auto font = vglx::Font::Create();

    font->AddGlyph(U'b', {});

    EXPECT_FALSE(font->GetGlyph(U'a').has_value());
    EXPECT_FALSE(font->GetGlyph(U'c').has_value());
}

#pragma endregion

#pragma region Kerning

TEST(Font, KerningIsDirectional) {
    auto font = vglx::Font::Create();

    font->AddKerning(U'A', U'V', -2.0f);

    EXPECT_FLOAT_EQ(font->GetKerning(U'A', U'V'), -2.0f);
    EXPECT_FLOAT_EQ(font->GetKerning(U'V', U'A'), 0.0f);
}

TEST(Font, AddKerningOverwritesExistingEntry) {
    auto font = vglx::Font::Create();

    font->AddKerning(U'A', U'V', -2.0f);
    font->AddKerning(U'A', U'V', -5.0f);

    EXPECT_FLOAT_EQ(font->GetKerning(U'A', U'V'), -5.0f);
}

TEST(Font, GetKerningReturnsZeroForMissingPair) {
    auto font = vglx::Font::Create();

    font->AddKerning(U'A', U'V', -2.0f);

    EXPECT_FLOAT_EQ(font->GetKerning(U'A', U'W'), 0.0f);
    EXPECT_FLOAT_EQ(font->GetKerning(char32_t {0}, U'A'), 0.0f);
}

#pragma endregion
