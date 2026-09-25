/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/canvas/text.hpp"

#include "vglx/geometries/buffer_attribute.hpp"
#include "vglx/textures/image.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace vglx {

namespace {

auto decode_utf8(std::string_view text) -> std::vector<char32_t> {
    auto out = std::vector<char32_t> {};
    out.reserve(text.size());

    for (auto i = std::size_t {0}; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i]);
        auto length = std::size_t {0};
        auto code = char32_t {0};

        if (lead < 0x80) {
            length = 1;
            code = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            length = 2;
            code = lead & 0x1F;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
            code = lead & 0x0F;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
            code = lead & 0x07;
        } else {
            ++i;
            continue;
        }

        if (i + length > text.size()) break;

        auto valid = true;
        for (auto k = std::size_t {1}; k < length; ++k) {
            const auto byte = static_cast<unsigned char>(text[i + k]);
            if ((byte & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            code = (code << 6) | (byte & 0x3F);
        }

        if (valid) out.push_back(code);
        i += valid ? length : 1;
    }

    return out;
}

auto split_lines(const std::vector<char32_t>& codes) -> std::vector<std::vector<char32_t>> {
    auto lines = std::vector<std::vector<char32_t>>(1);
    for (const auto code : codes) {
        if (code == U'\n') {
            lines.emplace_back();
        } else if (code != U'\r') {
            lines.back().push_back(code);
        }
    }
    return lines;
}

auto measure_line(const Font& font, const std::vector<char32_t>& line) -> float {
    auto width = 0.0f;
    for (auto i = std::size_t {0}; i < line.size(); ++i) {
        const auto glyph = font.GetGlyph(line[i]);
        if (!glyph.has_value()) continue;

        width += glyph->advance;
        if (i + 1 < line.size()) {
            width += font.GetKerning(line[i], line[i + 1]);
        }
    }
    return width;
}

}

Text::Text(std::shared_ptr<Font> font, std::string_view text) : font_(std::move(font)), text_(text) {
    Layout();
}

auto Text::EnsureCapacity(std::size_t glyphs) -> void {
    const auto required = std::max<std::size_t>(glyphs, 1);
    const auto reusable = geometry_ != nullptr && !geometry_->Disposed();
    if (reusable && required <= capacity_) return;

    // Attributes cannot be resized, so allocate room for the glyphs with
    // headroom and pad the unused quads. Dropping the previous geometry
    // releases its GPU buffers through its dispose callbacks.
    capacity_ = std::max(required, capacity_ * 2);
    geometry_ = Geometry::Create();

    geometry_->AddAttribute(BufferAttribute::Create({
        .name = BufferAttribute::kPosition,
        .format = BufferAttribute::Format::Float32x3,
        .rate = BufferAttribute::Rate::Vertex
    }, std::vector<float>(capacity_ * 12, 0.0f)));

    geometry_->AddAttribute(BufferAttribute::Create({
        .name = BufferAttribute::kTexCoord,
        .format = BufferAttribute::Format::Float32x2,
        .rate = BufferAttribute::Rate::Vertex
    }, std::vector<float>(capacity_ * 8, 0.0f)));
}

auto Text::SetText(std::string_view text) -> void {
    if (text_ == text) return;
    text_ = text;
    Layout();
}

auto Text::SetFont(std::shared_ptr<Font> font) -> void {
    if (font_ == font) return;
    font_ = std::move(font);
    Layout();
}

auto Text::SetAlignment(Alignment alignment) -> void {
    if (alignment_ == alignment) return;
    alignment_ = alignment;
    Layout();
}

auto Text::SetLineSpacing(float spacing) -> void {
    if (line_spacing_ == spacing) return;
    line_spacing_ = spacing;
    Layout();
}

auto Text::Layout() -> void {
    auto positions = std::vector<float> {};
    auto texcoords = std::vector<float> {};
    auto indices = std::vector<uint32_t> {};
    size_ = {0.0f, 0.0f};

    const auto codes = decode_utf8(text_);

    if (font_ != nullptr && !codes.empty()) {
        const auto lines = split_lines(codes);

        auto widths = std::vector<float> {};
        widths.reserve(lines.size());
        for (const auto& line : lines) {
            widths.push_back(measure_line(*font_, line));
        }

        const auto line_step = font_->line_height * line_spacing_;
        size_ = {
            *std::ranges::max_element(widths),
            font_->line_height + static_cast<float>(lines.size() - 1) * line_step
        };

        const auto page = font_->page != nullptr ? font_->page->image : nullptr;
        const auto page_size = page != nullptr
            ? Vector2 {static_cast<float>(page->width), static_cast<float>(page->height)}
            : Vector2 {0.0f, 0.0f};

        const auto align_factor = alignment_ == Alignment::Left ? 0.0f
            : alignment_ == Alignment::Center ? 0.5f : 1.0f;

        auto pen_y = 0.0f;
        for (auto l = std::size_t {0}; l < lines.size(); ++l) {
            const auto& line = lines[l];
            auto pen_x = (size_.x - widths[l]) * align_factor;

            for (auto i = std::size_t {0}; i < line.size(); ++i) {
                const auto glyph = font_->GetGlyph(line[i]);
                if (!glyph.has_value()) continue;

                const auto& r = glyph->region;
                const auto drawable = r.width > 0.0f && r.height > 0.0f
                    && page_size.x > 0.0f && page_size.y > 0.0f;

                if (drawable) {
                    const auto x0 = pen_x + glyph->offset.x;
                    const auto y0 = pen_y + glyph->offset.y;
                    const auto x1 = x0 + r.width;
                    const auto y1 = y0 + r.height;

                    const auto u0 = r.x / page_size.x;
                    const auto u1 = (r.x + r.width) / page_size.x;
                    const auto v0 = 1.0f - r.y / page_size.y;
                    const auto v1 = 1.0f - (r.y + r.height) / page_size.y;

                    const auto base = static_cast<uint32_t>(positions.size() / 3);

                    positions.insert(positions.end(), {
                        x0, y0, 0.0f,
                        x1, y0, 0.0f,
                        x1, y1, 0.0f,
                        x0, y1, 0.0f
                    });

                    texcoords.insert(texcoords.end(), {
                        u0, v0,
                        u1, v0,
                        u1, v1,
                        u0, v1
                    });

                    indices.insert(indices.end(), {
                        base, base + 2, base + 1,
                        base, base + 3, base + 2
                    });
                }

                pen_x += glyph->advance;
                if (i + 1 < line.size()) {
                    pen_x += font_->GetKerning(line[i], line[i + 1]);
                }
            }

            pen_y += line_step;
        }
    }

    EnsureCapacity(positions.size() / 12);
    positions.resize(capacity_ * 12, 0.0f);
    texcoords.resize(capacity_ * 8, 0.0f);

    geometry_->GetAttribute(BufferAttribute::kPosition)->SetData(std::move(positions));
    geometry_->GetAttribute(BufferAttribute::kTexCoord)->SetData(std::move(texcoords));
    geometry_->SetIndices(std::move(indices));
}

}
