/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "vglx/textures/texture.hpp"

/**
 * @defgroup LoadersGroup Loaders
 * @brief Free functions for loading external resources.
 */

namespace vglx {

namespace fs = std::filesystem;

class CubeTexture;
class Font;
class Node;
class Texture2D;
struct Image;

struct CubeTexturePaths {
    fs::path positive_x;
    fs::path negative_x;
    fs::path positive_y;
    fs::path negative_y;
    fs::path positive_z;
    fs::path negative_z;
};

/**
 * @brief Loads raw image data from disk.
 *
 * @code
 * auto image = vglx::LoadImage("assets/heightmap.png");
 * if (image.has_value()) {
 *     // use image.value()
 * } else {
 *     std::println(stderr, "{}", image.error());
 * }
 * @endcode
 *
 * @param path Filesystem path to the image asset.
 *
 * @ingroup LoadersGroup
 */
[[nodiscard]] VGLX_EXPORT auto LoadImage(
    const fs::path& path
) -> std::expected<std::shared_ptr<Image>, std::string>;

/**
 * @brief Loads raw HDR image data from disk.
 *
 * Loads a high dynamic range image (e.g. Radiance `.hdr`) as an
 * @ref Image whose pixel data is stored as 32-bit floats. For 8-bit
 * LDR images use @ref LoadImage.
 *
 * @code
 * auto image = vglx::LoadHDRImage("assets/sunset.hdr");
 * if (image.has_value()) {
 *     // use image.value()
 * } else {
 *     std::println(stderr, "{}", image.error());
 * }
 * @endcode
 *
 * @param path Filesystem path to the HDR image asset.
 *
 * @ingroup LoadersGroup
 */
[[nodiscard]] VGLX_EXPORT auto LoadHDRImage(
    const fs::path& path
) -> std::expected<std::shared_ptr<Image>, std::string>;

/**
 * @brief Loads a 2D texture from disk.
 *
 * @code
 * auto texture = vglx::LoadTexture("assets/crate.png");
 * if (texture.has_value()) {
 *     // use texture.value()
 * } else {
 *     std::println(stderr, "{}", texture.error());
 * }
 * @endcode
 *
 * @param path Filesystem path to the texture asset.
 * @param color_space Color space the texture is interpreted in.
 *
 * @ingroup LoadersGroup
 */
[[nodiscard]] VGLX_EXPORT auto LoadTexture(
    const fs::path& path,
    Texture::ColorSpace color_space = Texture::ColorSpace::sRGB
) -> std::expected<std::shared_ptr<Texture2D>, std::string>;

/**
 * @brief Loads an HDR 2D texture from disk.
 *
 * Loads a high dynamic range image (e.g. Radiance `.hdr`) as a
 * floating-point @ref Texture2D. The texture's color space is always
 * @ref Texture::ColorSpace::Linear since HDR data is by definition
 * stored in linear space. The mapping mode defaults to
 * @ref Texture::Mapping::Equirectangular since HDR images are typically
 * panoramas used as backgrounds or environments. Set @ref Texture::mapping
 * to @ref Texture::Mapping::UV for regular texture sampling.
 *
 * @code
 * auto texture = vglx::LoadHDRTexture("assets/sunset.hdr");
 * if (texture.has_value()) {
 *     // use texture.value()
 * } else {
 *     std::println(stderr, "{}", texture.error());
 * }
 * @endcode
 *
 * @param path Filesystem path to the HDR texture asset.
 *
 * @ingroup LoadersGroup
 */
[[nodiscard]] VGLX_EXPORT auto LoadHDRTexture(
    const fs::path& path
) -> std::expected<std::shared_ptr<Texture2D>, std::string>;

/**
 * @brief Loads a cube texture from six face images on disk.
 *
 * @code
 * auto skybox = vglx::LoadCubeTexture({
 *     .positive_x = "assets/skybox/px.jpg",
 *     .negative_x = "assets/skybox/nx.jpg",
 *     .positive_y = "assets/skybox/py.jpg",
 *     .negative_y = "assets/skybox/ny.jpg",
 *     .positive_z = "assets/skybox/pz.jpg",
 *     .negative_z = "assets/skybox/nz.jpg",
 * });
 *
 * if (skybox.has_value()) {
 *     // use skybox.value()
 * } else {
 *     std::println(stderr, "{}", skybox.error());
 * }
 * @endcode
 *
 * @param paths Filesystem paths to the six face images.
 * @param color_space Color space the cube texture is interpreted in.
 *
 * @ingroup LoadersGroup
 */
[[nodiscard]] VGLX_EXPORT auto LoadCubeTexture(
    const CubeTexturePaths& paths,
    Texture::ColorSpace color_space = Texture::ColorSpace::sRGB
) -> std::expected<std::shared_ptr<CubeTexture>, std::string>;

/**
 * @brief Loads a mesh asset from disk.
 *
 * @code
 * auto root = vglx::LoadMesh("assets/robot/robot.obj");
 * if (root.has_value()) {
 *     // use root.value()
 * } else {
 *     std::println(stderr, "{}", root.error());
 * }
 * @endcode
 *
 * @param path Filesystem path to the mesh asset.
 *
 * @ingroup LoadersGroup
 */
[[nodiscard]] VGLX_EXPORT auto LoadMesh(
    const fs::path& path
) -> std::expected<std::unique_ptr<Node>, std::string>;

/**
 * @brief Loads a font asset from disk.
 *
 * Supports single-page bitmap fonts in the text `.fnt` format exported by BMFont.
 * Pages that store glyph coverage in a color channel, such as 8-bit exports, are
 * converted on load so coverage lives in alpha and the color is white.
 *
 * @code
 * auto font = vglx::LoadFont("assets/fonts/futura_condensed.fnt");
 * if (font.has_value()) {
 *     // use font.value()
 * } else {
 *     std::println(stderr, "{}", font.error());
 * }
 * @endcode
 *
 * @param path Filesystem path to the font asset.
 *
 * @ingroup LoadersGroup
 */
[[nodiscard]] VGLX_EXPORT auto LoadFont(
    const fs::path& path
) -> std::expected<std::shared_ptr<Font>, std::string>;

}
