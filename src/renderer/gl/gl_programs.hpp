/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "core/program_attributes.hpp"
#include "core/shader_library.hpp"
#include "renderer/gl/gl_program.hpp"

#include <memory>
#include <unordered_map>

namespace vglx {

class GLPrograms {
public:
    auto GetProgram(const ProgramAttributes& attrs) -> GLProgram*;

    auto GetCanvasProgram() -> GLProgram*;

private:
    ShaderLibrary shader_lib_ {};

    std::unordered_map<std::size_t, std::unique_ptr<GLProgram>> programs_ {};

    std::unique_ptr<GLProgram> canvas_program_ {nullptr};
};

}