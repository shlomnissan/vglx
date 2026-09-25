/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/canvas/font.hpp>
#include <vglx/canvas/text.hpp>
#include <vglx/geometries/buffer_attribute.hpp>
#include <vglx/textures/image.hpp>
#include <vglx/textures/texture_2d.hpp>

#include <vector>

namespace {

auto make_font() {
    auto font = vglx::Font::Create();
    font->line_height = 16.0f;
    font->base = 12.0f;
    font->page = vglx::Texture2D::Create(vglx::Image::Create({.width = 64, .height = 64}));

    font->AddGlyph(U'A', {.region = {0.0f, 0.0f, 8.0f, 10.0f}, .offset = {1.0f, 2.0f}, .advance = 10.0f});
    font->AddGlyph(U'B', {.region = {8.0f, 0.0f, 8.0f, 10.0f}, .offset = {0.0f, 2.0f}, .advance = 9.0f});
    font->AddKerning(U'A', U'B', -2.0f);

    return font;
}

auto positions(vglx::Text& text) -> const std::vector<float>& {
    return text.GetGeometry()->GetAttribute(vglx::BufferAttribute::kPosition)->GetData();
}

auto texcoords(vglx::Text& text) -> const std::vector<float>& {
    return text.GetGeometry()->GetAttribute(vglx::BufferAttribute::kTexCoord)->GetData();
}

}

#pragma region Measurement

TEST(Text, SizeSumsAdvancesAndKerning) {
    auto text = vglx::Text::Create(make_font(), "AB");

    EXPECT_VEC2_EQ(text->GetSize(), {17.0f, 16.0f});
}

TEST(Text, SizeSpansLines) {
    auto text = vglx::Text::Create(make_font(), "AB\nA");

    EXPECT_VEC2_EQ(text->GetSize(), {17.0f, 32.0f});
}

TEST(Text, SizeIsZeroWhenEmptyOrWithoutFont) {
    EXPECT_VEC2_EQ(vglx::Text::Create(make_font(), "")->GetSize(), {0.0f, 0.0f});
    EXPECT_VEC2_EQ(vglx::Text::Create(nullptr, "AB")->GetSize(), {0.0f, 0.0f});
}

#pragma endregion

#pragma region Layout

TEST(Text, LayoutPlacesGlyphsAtPenPlusOffset) {
    auto text = vglx::Text::Create(make_font(), "AB");

    EXPECT_EQ(positions(*text), (std::vector<float> {
         1.0f,  2.0f, 0.0f,
         9.0f,  2.0f, 0.0f,
         9.0f, 12.0f, 0.0f,
         1.0f, 12.0f, 0.0f,
         8.0f,  2.0f, 0.0f,
        16.0f,  2.0f, 0.0f,
        16.0f, 12.0f, 0.0f,
         8.0f, 12.0f, 0.0f
    }));
}

TEST(Text, LayoutFlipsVForTheAtlas) {
    auto text = vglx::Text::Create(make_font(), "A");

    EXPECT_EQ(texcoords(*text), (std::vector<float> {
        0.000f, 1.00000f,
        0.125f, 1.00000f,
        0.125f, 0.84375f,
        0.000f, 0.84375f
    }));
}

TEST(Text, NewlineDropsByLineHeight) {
    auto text = vglx::Text::Create(make_font(), "A\nA");
    const auto& p = positions(*text);

    EXPECT_FLOAT_EQ(p[1], 2.0f);
    EXPECT_FLOAT_EQ(p[12 + 1], 18.0f);
}

TEST(Text, LineSpacingScalesTheDrop) {
    auto text = vglx::Text::Create(make_font(), "A\nA");
    text->SetLineSpacing(1.5f);

    EXPECT_FLOAT_EQ(positions(*text)[12 + 1], 26.0f);
    EXPECT_VEC2_EQ(text->GetSize(), {10.0f, 40.0f});
}

TEST(Text, RightAlignShiftsShorterLines) {
    auto text = vglx::Text::Create(make_font(), "AB\nA");

    text->SetAlignment(vglx::Text::Alignment::Right);
    EXPECT_FLOAT_EQ(positions(*text)[24], 8.0f);
}

TEST(Text, MissingGlyphsAreSkippedWithoutAdvance) {
    auto text = vglx::Text::Create(make_font(), "A?B");

    EXPECT_VEC2_EQ(text->GetSize(), {19.0f, 16.0f});
    EXPECT_FLOAT_EQ(positions(*text)[12], 10.0f);
}

TEST(Text, EmptyTextHasNoIndices) {
    auto text = vglx::Text::Create(make_font(), "");

    EXPECT_TRUE(text->GetGeometry()->GetIndexData().empty());
}

#pragma endregion

#pragma region Capacity

TEST(Text, GrowingTextReplacesGeometry) {
    auto text = vglx::Text::Create(make_font(), "A");
    const auto before = text->GetGeometry();

    text->SetText("ABAB");

    EXPECT_NE(text->GetGeometry(), before);
    EXPECT_EQ(positions(*text).size(), 4 * 12);
    EXPECT_EQ(text->GetGeometry()->GetIndexData().size(), 4 * 6);

    EXPECT_FLOAT_EQ(positions(*text)[36], 25.0f);
}

TEST(Text, ShrinkingTextKeepsGeometryAndPadsQuads) {
    auto text = vglx::Text::Create(make_font(), "ABAB");
    const auto before = text->GetGeometry();

    text->SetText("A");

    EXPECT_EQ(text->GetGeometry(), before);
    EXPECT_EQ(positions(*text).size(), 4 * 12);
    EXPECT_EQ(text->GetGeometry()->GetIndexData().size(), 6);

    EXPECT_FLOAT_EQ(positions(*text)[12], 0.0f);
    EXPECT_FLOAT_EQ(positions(*text)[36], 0.0f);
}

#pragma endregion
