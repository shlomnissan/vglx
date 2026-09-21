/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/loaders.hpp"

#include "vglx/canvas/font.hpp"

#include "loaders/detail/bmfont_import.hpp"

#include <format>

namespace vglx {

namespace {

auto load_bmfont(const fs::path& path) -> std::expected<std::shared_ptr<Font>, std::string> {
    auto result = detail::bmfont::import(path);
    if (!result) {
        return std::unexpected(result.error());
    }

    return std::unexpected("not implemented");
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
