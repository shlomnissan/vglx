/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "vglx/math/rect.hpp"
#include "vglx/math/vector2.hpp"

namespace vglx::detail::bmfont {

namespace fs = std::filesystem;

struct BMFontResult {
    struct Char {
        int id {0};
        float advance {0.0f};
        Rect region;
        Vector2 offset;
    };

    struct Kerning {
        int first {0};
        int second {0};
        float amount {0.0f};
    };

    enum class Channel { Red, Green, Blue, Alpha };

    std::string font_face;
    fs::path page;

    Channel glyph_channel {Channel::Alpha};

    float size {0.0f};
    float line_height {0.0f};
    float base {0.0f};

    std::vector<Char> chars;
    std::vector<Kerning> kernings;
};

[[nodiscard]] auto import(const fs::path& path) -> std::expected<BMFontResult, std::string>;

}
