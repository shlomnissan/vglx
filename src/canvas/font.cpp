/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/canvas/font.hpp"

#include "utilities/logger.hpp"

#include <algorithm>

namespace vglx {

namespace {

auto kerning_key(char32_t first, char32_t second) -> uint64_t {
    return static_cast<uint64_t>(first) << 32 | static_cast<uint64_t>(second);
}

}

auto Font::AddGlyph(char32_t code, const Glyph& glyph) -> void {
    auto it = std::ranges::lower_bound(glyphs_, code, {}, &std::pair<char32_t, Glyph>::first);

    if (it != glyphs_.end() && it->first == code) {
        Logger::Log(
            LogLevel::Warning,
            "Adding glyph {} overrides an existing entry in font {}",
            static_cast<uint32_t>(code),
            DisplayName()
        );
        it->second = glyph;
        return;
    }

    glyphs_.insert(it, {code, glyph});
}

auto Font::AddKerning(char32_t first, char32_t second, float amount) -> void {
    const auto key = kerning_key(first, second);
    auto it = std::ranges::lower_bound(kerning_, key, {}, &std::pair<uint64_t, float>::first);

    if (it != kerning_.end() && it->first == key) {
        Logger::Log(
            LogLevel::Warning,
            "Adding kerning pair {}, {} overrides an existing entry in font {}",
            static_cast<uint32_t>(first),
            static_cast<uint32_t>(second),
            DisplayName()
        );
        it->second = amount;
        return;
    }

    kerning_.insert(it, {key, amount});
}

auto Font::GetGlyph(char32_t code) const -> std::optional<Glyph> {
    auto it = std::ranges::lower_bound(glyphs_, code, {}, &std::pair<char32_t, Glyph>::first);
    if (it != glyphs_.end() && it->first == code) return it->second;
    return std::nullopt;
}

auto Font::GetKerning(char32_t first, char32_t second) const -> float {
    const auto key = kerning_key(first, second);
    auto it = std::ranges::lower_bound(kerning_, key, {}, &std::pair<uint64_t, float>::first);
    if (it != kerning_.end() && it->first == key) return it->second;
    return 0.0f;
}

}
