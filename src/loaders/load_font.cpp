/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/loaders.hpp"

#include "vglx/canvas/font.hpp"
#include "vglx/textures/image.hpp"
#include "vglx/textures/texture_2d.hpp"

#include "loaders/detail/bmfont_import.hpp"

#include <cstdint>
#include <format>
#include <variant>
#include <vector>

namespace vglx {

namespace {

using BMFontChannel = detail::bmfont::BMFontResult::Channel;

auto move_coverage_to_alpha(Image& image, BMFontChannel channel) -> void {
    auto pixels = std::get_if<std::vector<uint8_t>>(&image.data);
    if (pixels == nullptr) {
        return;
    }

    const auto source = static_cast<std::size_t>(channel);
    for (auto i = std::size_t {0}; i + 3 < pixels->size(); i += 4) {
        const auto coverage = (*pixels)[i + source];
        (*pixels)[i] = 255;
        (*pixels)[i + 1] = 255;
        (*pixels)[i + 2] = 255;
        (*pixels)[i + 3] = coverage;
    }
}

auto load_bmfont(const fs::path& path) -> std::expected<std::shared_ptr<Font>, std::string> {
    auto result = detail::bmfont::import(path);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    auto page = LoadTexture(result->page, Texture::ColorSpace::Linear);
    if (!page.has_value()) {
        return std::unexpected(page.error());
    }

    if (result->glyph_channel != BMFontChannel::Alpha) {
        move_coverage_to_alpha(*page.value()->image, result->glyph_channel);
    }

    page.value()->wrap_s = Texture::Wrapping::ClampToEdge;
    page.value()->wrap_t = Texture::Wrapping::ClampToEdge;

    auto font = Font::Create();
    font->SetName(result->font_face);
    font->size = result->size;
    font->line_height = result->line_height;
    font->base = result->base;
    font->page = page.value();

    for (const auto& c : result->chars) {
        // BMFont emits id=-1 for its "invalid char" fallback glyph, which
        // has no code point to key on.
        if (c.id < 0) continue;

        font->AddGlyph(static_cast<char32_t>(c.id), {
            .region = c.region,
            .offset = c.offset,
            .advance = c.advance
        });
    }

    for (const auto& k : result->kernings) {
        font->AddKerning(
            static_cast<char32_t>(k.first),
            static_cast<char32_t>(k.second),
            k.amount
        );
    }

    return font;
}

}

auto LoadFont(
    const fs::path& path
) -> std::expected<std::shared_ptr<Font>, std::string> {
    if (!fs::exists(path)) {
        return std::unexpected(
            std::format(
                "Load font failed: cannot find file {}",
                path.string()
            )
        );
    }

    if (path.extension().string() != ".fnt") {
        return std::unexpected(
            std::format(
                "Load font failed: unsupported file extension {}. "
                "Currently, only single page bitmap fonts exported from BMFont are supported.",
                path.extension().string()
            )
        );
    }

    return load_bmfont(path);
}

}
