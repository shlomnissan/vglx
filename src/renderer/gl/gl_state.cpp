/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/gl/gl_state.hpp"

#include "vglx/materials/material.hpp"

#include "utilities/assert.hpp"

#include <glad/glad.h>

namespace vglx {

auto GLState::ProcessMaterial(const Material* material) -> void {
    SetBlending(!material->transparent ? Material::Blending::None : material->blending);
    SetDepthFunction(material->depth);
    SetDepthTest(material->depth_test);
    SetPolygonOffset(material->polygon_offset_factor, material->polygon_offset_units);
    SetSide(material->side);
}

auto GLState::Enable(int token) -> void {
    if (!features_.contains(token) || !features_[token]) {
        glEnable(token);
        features_[token] = true;
    }
}

auto GLState::Disable(int token) -> void {
    if (features_.contains(token) && features_[token]) {
        glDisable(token);
        features_[token] = false;
    }
}

auto GLState::SetViewport(int x, int y, int width, int height) const -> void {
    glViewport(x, y, width, height);
}

auto GLState::SetSide(Material::Side side) -> void {
    if (side == Material::Side::TwoSided) {
        Disable(GL_CULL_FACE);
        return;
    }

    Enable(GL_CULL_FACE);

    const auto cull_front = side == Material::Side::Back;
    if (curr_cull_front_ != cull_front) {
        glCullFace(cull_front ? GL_FRONT : GL_BACK);
        curr_cull_front_ = cull_front;
    }
}

auto GLState::SetDepthTest(bool enabled) -> void {
    enabled ? Enable(GL_DEPTH_TEST) : Disable(GL_DEPTH_TEST);
}

auto GLState::SetDepthWrites(bool enabled) -> void {
    if (curr_depth_mask_ != enabled) {
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
        curr_depth_mask_ = enabled;
    }
}

auto GLState::SetSeamlessCubemapFiltering() -> void {
    Enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

auto GLState::UseProgram(unsigned int program_id) -> void {
    if (curr_program_ != program_id) {
        glUseProgram(program_id);
        curr_program_ = program_id;
    }
}

auto GLState::SetPolygonOffset(float factor, float units) -> void {
    if (factor != 0.0f || units != 0.0f) {
        Enable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(factor, units);
    } else {
        Disable(GL_POLYGON_OFFSET_FILL);
    }
}

auto GLState::SetBlending(Material::Blending blending) -> void {
    using enum Material::Blending;
    if (curr_blending_ != blending) {
        if (blending == None) {
            Disable(GL_BLEND);
        } else {
            Enable(GL_BLEND);
            switch (blending) {
                case Normal:
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case Additive:
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
                    break;
                case Subtractive:
                    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                    break;
                case Multiply:
                    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
                    break;
                case None: break;
                default: VGLX_UNREACHABLE();
            }
        }
        curr_blending_ = blending;
    }
}

auto GLState::SetDepthFunction(Material::Depth depth) -> void {
    using enum Material::Depth;
    if (curr_depth_ != depth) {
        switch(depth) {
            case Less: glDepthFunc(GL_LESS); break;
            case Equal: glDepthFunc(GL_EQUAL); break;
            case LessEqual: glDepthFunc(GL_LEQUAL); break;
            case Greater: glDepthFunc(GL_GREATER); break;
            case GreaterEqual: glDepthFunc(GL_GEQUAL); break;
            case Always: glDepthFunc(GL_ALWAYS); break;
            case Never: glDepthFunc(GL_NEVER); break;
            default: VGLX_UNREACHABLE();
        }
        curr_depth_ = depth;
    }
}

auto GLState::SetClearColor(const Color& color) -> void {
    if (curr_clear_color_ != color) {
        glClearColor(color.r, color.g, color.b, 1.0f);
        curr_clear_color_ = color;
    }
}

auto GLState::Reset() -> void {
    SetSide(Material::Side::TwoSided);
    SetDepthTest(false);
    SetPolygonOffset(0.0f, 0.0f);
    SetBlending(Material::Blending::None);
    SetDepthFunction(Material::Depth::LessEqual);

    curr_program_ = 0;
}

}
