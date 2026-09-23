/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/gl/gl_scene_buffer.hpp"

#include "renderer/gl/gl_device.hpp"
#include "utilities/assert.hpp"
#include "utilities/logger.hpp"

#include <glad/glad.h>

#include <algorithm>

namespace vglx {

namespace {

constexpr GLenum kColorFormat = GL_RGBA16F;
constexpr GLenum kDepthStencilFormat = GL_DEPTH24_STENCIL8;

}

struct GLSceneBuffer::Impl {
    GLuint resolve_fbo {0};
    GLuint resolve_color {0};
    GLuint resolve_depth_stencil {0};
    GLuint msaa_fbo {0};
    GLuint msaa_color {0};
    GLuint msaa_depth_stencil {0};

    int width {0};
    int height {0};
    int samples {0};

    bool is_msaa {false};

    explicit Impl(int sample_count) : samples(std::max(sample_count, 1)), is_msaa(samples > 1) {}

    auto Initialize(int width, int height) -> std::expected<void, std::string> {
        this->width = width;
        this->height = height;

        if (width <= 0 || height <= 0) {
            return std::unexpected("Scene buffer invalid size");
        }

        const auto requested_samples = samples;
        if (is_msaa) {
            samples = std::min(samples, gl::limits().max_samples);
        }

        DeleteBuffers();

        glGenFramebuffers(1, &resolve_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo);

        glGenTextures(1, &resolve_color);
        glBindTexture(GL_TEXTURE_2D, resolve_color);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            kColorFormat,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            resolve_color,
            0
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            UnbindBuffers();
            DeleteBuffers();
            Logger::Log(LogLevel::Error, "Failed to create a scene buffer");
            return std::unexpected("Failed to create a scene buffer");
        }

        return is_msaa ? InitWithMSAA(requested_samples) : InitWithoutMSAA();
    }

    auto InitWithoutMSAA() -> std::expected<void, std::string> {
        glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo);

        glGenRenderbuffers(1, &resolve_depth_stencil);
        glBindRenderbuffer(GL_RENDERBUFFER, resolve_depth_stencil);
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            kDepthStencilFormat,
            width,
            height
        );

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            resolve_depth_stencil
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            UnbindBuffers();
            DeleteBuffers();
            Logger::Log(LogLevel::Error, "Failed to create a scene buffer");
            return std::unexpected("Failed to create a scene buffer");
        }

        UnbindBuffers();

        return {};
    }

    auto InitWithMSAA(int requested_samples) -> std::expected<void, std::string> {
        glGenFramebuffers(1, &msaa_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);

        glGenRenderbuffers(1, &msaa_color);
        glBindRenderbuffer(GL_RENDERBUFFER, msaa_color);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            samples,
            kColorFormat,
            width,
            height
        );

        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);
        if (requested_samples != samples) {
            Logger::Log(
                LogLevel::Warning, "Sample count mismatch "
                "(requested: {}, resolved: {}).",
                requested_samples, samples
            );
        }

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER,
            msaa_color
        );

        glGenRenderbuffers(1, &msaa_depth_stencil);
        glBindRenderbuffer(GL_RENDERBUFFER, msaa_depth_stencil);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            samples,
            kDepthStencilFormat,
            width,
            height
        );

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            msaa_depth_stencil
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            UnbindBuffers();
            DeleteBuffers();
            Logger::Log(LogLevel::Error, "Failed to create a scene buffer with MSAA");
            return std::unexpected("Failed to create a scene buffer with MSAA");
        }

        UnbindBuffers();

        return {};
    }

    auto ResizeViewport(int width, int height) -> void {
        if (width <= 0 || height <= 0) return;

        if (resolve_color == 0) return;

        if (width == this->width && height == this->height) return;

        glBindTexture(GL_TEXTURE_2D, resolve_color);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            kColorFormat,
            width,
            height,
            0,
            GL_RGBA,
            GL_HALF_FLOAT,
            nullptr
        );
        glBindTexture(GL_TEXTURE_2D, 0);

        if (!is_msaa) {
            glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo);

            glBindRenderbuffer(GL_RENDERBUFFER, resolve_depth_stencil);
            glRenderbufferStorage(
                GL_RENDERBUFFER,
                kDepthStencilFormat,
                width,
                height
            );
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            VGLX_ASSERT(
                status == GL_FRAMEBUFFER_COMPLETE,
                "Scene buffer incomplete after resize (no MSAA)"
            );
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);

            glBindRenderbuffer(GL_RENDERBUFFER, msaa_color);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                kColorFormat,
                width,
                height
            );

            glBindRenderbuffer(GL_RENDERBUFFER, msaa_depth_stencil);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                kDepthStencilFormat,
                width,
                height
            );

            auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            VGLX_ASSERT(
                status == GL_FRAMEBUFFER_COMPLETE,
                "Scene buffer incomplete after resize (MSAA)"
            );
        }

        this->width = width;
        this->height = height;
    }

    auto Begin() const -> void {
        glBindFramebuffer(GL_FRAMEBUFFER, is_msaa ? msaa_fbo : resolve_fbo);
        glViewport(0, 0, width, height);
    }

    auto End() const -> void {
        if (is_msaa) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo);
            glBlitFramebuffer(
                0, 0, width, height,
                0, 0, width, height,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    auto UnbindBuffers() const -> void {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    auto DeleteBuffers() -> void {
        if (resolve_fbo) glDeleteFramebuffers(1, &resolve_fbo);
        if (resolve_color) glDeleteTextures(1, &resolve_color);
        if (resolve_depth_stencil) glDeleteRenderbuffers(1, &resolve_depth_stencil);
        if (msaa_fbo) glDeleteFramebuffers(1, &msaa_fbo);
        if (msaa_color) glDeleteRenderbuffers(1, &msaa_color);
        if (msaa_depth_stencil) glDeleteRenderbuffers(1, &msaa_depth_stencil);

        resolve_fbo = 0;
        resolve_color = 0;
        resolve_depth_stencil = 0;
        msaa_fbo = 0;
        msaa_color = 0;
        msaa_depth_stencil = 0;
    }
};

GLSceneBuffer::GLSceneBuffer(int sample_count) : impl_(std::make_unique<GLSceneBuffer::Impl>(sample_count)) {}

auto GLSceneBuffer::Initialize(int width, int height) -> std::expected<void, std::string> {
    return impl_->Initialize(width, height);
}

auto GLSceneBuffer::ResizeViewport(int width, int height) -> void {
    impl_->ResizeViewport(width, height);
}

auto GLSceneBuffer::Begin() const -> void {
    impl_->Begin();
}

auto GLSceneBuffer::End() const -> void {
    impl_->End();
}

auto GLSceneBuffer::GetResolvedColorTexture() const -> unsigned int {
    return impl_->resolve_color;
}


GLSceneBuffer::~GLSceneBuffer() {
    impl_->DeleteBuffers();
}

}
