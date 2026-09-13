/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/gl/gl_uniform_buffer.hpp"

#include "utilities/logger.hpp"

#include <cstring>

namespace vglx {

GLUniformBuffer::GLUniformBuffer(std::string_view name, std::size_t size) :
    name_(name),
    binding_point_(get_uniform_block_loc(name)),
    size_(size)
{
    if (binding_point_ == -1) {
        Logger::Log(LogLevel::Error, "Unknown uniform block {}", name);
        return;
    }

    data_ = std::make_unique<std::byte[]>(size);

    glGenBuffers(1, &buffer_);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer_);
    glBufferData(GL_UNIFORM_BUFFER, size_, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_, buffer_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

GLUniformBuffer::GLUniformBuffer(GLUniformBuffer&& other) noexcept
    : name_(std::move(other.name_)),
      buffer_(other.buffer_),
      binding_point_(other.binding_point_),
      size_(other.size_),
      data_(std::move(other.data_)) {
    other.buffer_ = 0;
    other.binding_point_ = -1;
    other.size_ = 0;
}

auto GLUniformBuffer::operator=(GLUniformBuffer&& other) noexcept -> GLUniformBuffer& {
    if (this != &other) {
        if (buffer_ != 0) {
            glDeleteBuffers(1, &buffer_);
        }
        name_ = std::move(other.name_);
        buffer_ = other.buffer_;
        binding_point_ = other.binding_point_;
        size_ = other.size_;
        data_ = std::move(other.data_);

        other.buffer_ = 0;
        other.binding_point_ = -1;
        other.size_ = 0;
    }
    return *this;
}

auto GLUniformBuffer::UploadIfNeeded(const void* data, std::size_t size) const -> void {
    if (binding_point_ == -1) return;

    if (size > size_) {
        Logger::Log(LogLevel::Error, "UBO {} update size exceeds buffer size", name_);
        return;
    }
    if (std::memcmp(data_.get(), data, size) != 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, buffer_);

        auto mapped = glMapBufferRange(
            GL_UNIFORM_BUFFER,
            /* offset = */ 0,
            size,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
        );

        if (mapped) {
            std::memcpy(mapped, data, size);
            glUnmapBuffer(GL_UNIFORM_BUFFER);
        } else {
            Logger::Log(LogLevel::Error, "UBO {} map buffer failed", name_);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        std::memcpy(data_.get(), data, size);
    }
}

GLUniformBuffer::~GLUniformBuffer() {
    if (buffer_ != 0) {
        glDeleteBuffers(1, &buffer_);
        buffer_ = 0;
    }
}

}
