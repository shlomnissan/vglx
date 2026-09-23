/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include <expected>
#include <memory>
#include <string>

namespace vglx {

class GLSceneBuffer {
public:
    explicit GLSceneBuffer(int sample_count);

    GLSceneBuffer(const GLSceneBuffer&) = delete;
    GLSceneBuffer(GLSceneBuffer&&) = delete;

    auto operator=(const GLSceneBuffer&) -> GLSceneBuffer& = delete;
    auto operator=(GLSceneBuffer&&) -> GLSceneBuffer& = delete;

    [[nodiscard]] auto Initialize(int width, int height) -> std::expected<void, std::string>;

    auto ResizeViewport(int width, int height) -> void;

    auto Begin() const -> void;

    auto End() const -> void;

    ~GLSceneBuffer();

private:
    friend class GLPresentPass;
    class Impl;
    std::unique_ptr<Impl> impl_;

    [[nodiscard]] auto GetResolvedColorTexture() const -> unsigned int;
};

}