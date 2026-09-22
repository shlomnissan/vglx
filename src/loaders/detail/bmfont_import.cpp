/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "loaders/detail/bmfont_import.hpp"

#include <expected>
#include <format>
#include <fstream>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vglx::detail::bmfont {

namespace {

struct ParsedLineOutput {
    struct Token {
        std::string key;
        std::string value;
    };

    std::string type;
    std::vector<Token> tokens {};
};

auto parse_line(std::string_view line) -> std::expected<ParsedLineOutput, std::string> {
    if (line.ends_with('\r')) line.remove_suffix(1);

    auto type_end = line.find_first_of(" \t");
    if (type_end == std::string_view::npos) {
        return std::unexpected("malformed line missing type separator");
    }

    auto output = ParsedLineOutput {};
    output.type = line.substr(0, type_end);

    auto fields = line.substr(type_end + 1);
    while (!fields.empty()) {
        auto field_begin = fields.find_first_not_of(" \t");
        if (field_begin == std::string_view::npos) break;

        fields.remove_prefix(field_begin);

        auto divider = fields.find_first_of('=');
        if (divider == std::string_view::npos) {
            return std::unexpected("malformed field missing equals divider");
        }

        auto key = std::string {fields.substr(0, divider)};
        fields.remove_prefix(key.size() + 1);

        auto value = std::string {};
        if (fields.starts_with('"')) {
            fields.remove_prefix(1);
            auto closing_quote = fields.find_first_of('"');
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
                value = fields.substr(0,  field_end);
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

    file >> std::noskipws;

    auto text = std::views::istream<char>(file) | std::ranges::to<std::string>();
    if (text.empty()) return make_error("file is empty");
    if (text.starts_with("BMF")) {
        return make_error("binary format is unsupported");
    }

    auto lines = text | std::views::split('\n');
    auto iter = lines.begin();
    auto output = BMFontResult {};

    auto info = parse_line(std::string_view {*iter});
    if (!info.has_value()) return make_error(info.error());

    if (info->type != "info") {
        return make_error("missing info entry");
    }

    for (auto [key, value] : info->tokens) {
        if (key == "face") output.font_face = value;
        if (key == "size") output.size = std::stof(value);
    }

    // TODO: implement

    return std::unexpected("not implemented yet");
}

}
