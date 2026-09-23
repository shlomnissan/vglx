/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "loaders/detail/bmfont_import.hpp"

#include <charconv>
#include <expected>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace vglx::detail::bmfont {

namespace {

struct Fields {
    struct Token {
        std::string key;
        std::string value;
    };

    std::string type;
    std::vector<Token> tokens {};
    std::optional<std::string> error {};

    auto String(std::string_view key) -> std::string_view {
        for (const auto& token : tokens) {
            if (token.key == key) return token.value;
        }

        if (!error) {
            error = std::format("'{}' line is missing field '{}'", type, key);
        }

        return {};
    }

    auto Int(std::string_view key) -> int {
        auto value = String(key);
        auto result = 0;
        auto end = value.data() + value.size();
        auto [ptr, ec] = std::from_chars(value.data(), end, result);
        if (!error && (ec != std::errc {} || ptr != end)) {
            error = std::format("'{}' line has non-integer field {}={}", type, key, value);
        }
        return result;
    }

    auto Float(std::string_view key) -> float {
        return static_cast<float>(Int(key));
    }
};

auto parse_line(std::string_view line) -> std::expected<Fields, std::string> {
    if (line.ends_with('\r')) line.remove_suffix(1);

    auto type_end = line.find_first_of(" \t");
    if (type_end == std::string_view::npos) {
        return std::unexpected("malformed line missing type separator");
    }

    auto output = Fields {};
    output.type = line.substr(0, type_end);

    auto fields = line.substr(type_end + 1);
    while (!fields.empty()) {
        auto field_begin = fields.find_first_not_of(" \t");
        if (field_begin == std::string_view::npos) break;

        fields.remove_prefix(field_begin);

        auto divider = fields.find('=');
        if (divider == std::string_view::npos) {
            return std::unexpected("malformed field missing equals divider");
        }

        auto key = std::string {fields.substr(0, divider)};
        fields.remove_prefix(key.size() + 1);

        auto value = std::string {};
        if (fields.starts_with('"')) {
            fields.remove_prefix(1);
            auto closing_quote = fields.find('"');

            while (closing_quote != std::string_view::npos &&
                   closing_quote + 1 < fields.size() &&
                   fields[closing_quote + 1] != ' ' &&
                   fields[closing_quote + 1] != '\t') {
                closing_quote = fields.find('"', closing_quote + 1);
            }

            if (closing_quote == std::string_view::npos) {
                return std::unexpected("unterminated quoted value");
            }

            value = fields.substr(0, closing_quote);
            fields.remove_prefix(closing_quote + 1);
        } else {
            auto field_end = fields.find_first_of(" \t");
            if (field_end == std::string_view::npos) {
                value = fields;
                fields = {};
            } else {
                value = fields.substr(0, field_end);
                fields.remove_prefix(field_end);
            }
        }

        output.tokens.emplace_back(std::move(key), std::move(value));
    }

    return output;
}

}

auto import(const fs::path& path) -> std::expected<BMFontResult, std::string> {
    auto make_error = [&path](std::string_view message) {
        return std::unexpected(
            std::format("failed to import bitmap font {}: {}", path.string(), message)
        );
    };

    auto file = std::ifstream {path, std::ios::binary};
    if (!file) return make_error("cannot open file");

    auto text = std::string {std::istreambuf_iterator<char> {file}, {}};
    if (text.empty()) return make_error("file is empty");
    if (text.starts_with("BMF")) {
        return make_error("binary format is unsupported");
    }

    auto output = BMFontResult {};
    auto seen_info = false;
    auto seen_common = false;

    for (auto chunk : text | std::views::split('\n')) {
        auto line = std::string_view {chunk};
        if (line.find_first_not_of(" \t\r") == std::string_view::npos) continue;

        auto fields = parse_line(line);
        if (!fields.has_value()) {
            return make_error(fields.error());
        }

        if (fields->type == "info") {
            seen_info = true;
            output.font_face = fields->String("face");
            output.size = fields->Float("size");
        }

        if (fields->type == "common") {
            seen_common = true;
            output.line_height = fields->Float("lineHeight");
            output.base = fields->Float("base");
            if (fields->Int("pages") > 1) {
                return make_error("multi-page fonts are unsupported");
            }
        }

        if (fields->type == "page") {
            output.page = path.parent_path() / fields->String("file");
        }

        if (fields->type == "char") {
            output.chars.emplace_back(BMFontResult::Char {
                .id = fields->Int("id"),
                .advance = fields->Float("xadvance"),
                .region = {
                    fields->Float("x"),
                    fields->Float("y"),
                    fields->Float("width"),
                    fields->Float("height")
                },
                .offset = {
                    fields->Float("xoffset"),
                    fields->Float("yoffset")
                }
            });
        }

        if (fields->type == "kerning") {
            output.kernings.emplace_back(BMFontResult::Kerning {
                .first = fields->Int("first"),
                .second = fields->Int("second"),
                .amount = fields->Float("amount")
            });
        }

        if (fields->error) return make_error(*fields->error);
    }

    if (!seen_info) {
        return make_error("missing info entry");
    }

    if (!seen_common) {
        return make_error("missing common entry");
    }

    if (output.page.empty()) {
        return make_error("missing page entry");
    }

    return output;
}

}
