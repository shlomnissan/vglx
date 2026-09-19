/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/gl/gl_programs.hpp"

#include "utilities/logger.hpp"

namespace vglx {

auto GLPrograms::GetProgram(const ProgramAttributes& attrs) -> GLProgram* {
    const auto& key = attrs.key;
    if (!programs_.contains(key)) {
        auto sources = shader_lib_.GetShaderSource(attrs);
        if (sources.empty()) {
            return nullptr;
        }

        programs_[key] = std::make_unique<GLProgram>(sources);

        Logger::Log(
            LogLevel::Debug,
            "Created a new shader program {}:{}",
            key, Material::TypeToString(attrs.type)
        );

    }
    return programs_[key].get();
}

auto GLPrograms::GetCanvasProgram() -> GLProgram* {
    if (canvas_program_ == nullptr) {
        canvas_program_ = std::make_unique<GLProgram>(shader_lib_.GetCanvasShaderSource());

        Logger::Log(LogLevel::Debug, "Created the canvas shader program");
    }
    return canvas_program_.get();
}

}
