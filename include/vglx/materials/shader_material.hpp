/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/materials/material.hpp"
#include "vglx/math/color.hpp"
#include "vglx/math/matrix3.hpp"
#include "vglx/math/matrix4.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/math/vector3.hpp"
#include "vglx/math/vector4.hpp"
#include "vglx/textures/texture.hpp"

#include <memory>
#include <utility>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vglx {

/**
 * @brief Material rendered using user-defined GLSL shaders.
 *
 * Shader material provides full control over the shading pipeline by allowing
 * custom vertex and fragment shader code. It is intended for advanced
 * rendering effects, experimental lighting models, and post-processing passes
 * that go beyond built-in material types.
 *
 * Uniforms can be defined dynamically at creation time and later updated by
 * name. Supported uniform types include scalar, vector, matrix, and color
 * values.
 *
 * @code
 * auto material = vglx::ShaderMaterial::Create({
 *   .vertex_shader = vert_source,
 *   .fragment_shader = frag_source,
 *   .uniforms = {
 *     {"u_Intensity", 1.0f},
 *     {"u_Offset", Vector2::Zero()}
 *   }
 * });
 *
 * // Update a uniform after creation
 * material->SetUniform("u_Intensity", 0.5f);
 *
 * my_scene->Add(vglx::Mesh::Create(geometry, material));
 * @endcode
 *
 * @ingroup MaterialsGroup
 */
class VGLX_EXPORT ShaderMaterial : public Material {
public:
    /**
     * @brief Represents a supported uniform value type.
     *
     * A uniform can be a boolean, integer, float, color, or a vector/matrix type.
     * Values are matched by name when uploaded to the shader program.
     */
    using UniformValue = std::variant<bool, int, float, Color, Matrix3, Matrix4, Vector2, Vector3, Vector4>;

    /**
     * @brief List of named uniform initializers.
     *
     * Defines a collection of uniform name–value pairs used to initialize a
     * @ref ShaderMaterial at creation time. Each entry maps a GLSL uniform
     * variable name to its initial value.
     */
    using UniformList = std::initializer_list<std::pair<std::string, UniformValue>>;

    /**
     * @brief Represents a texture bound to a shader sampler.
     *
     * Maps a GLSL sampler name to a specific texture instance. Texture bindings
     * are used to pass image data to the shader for sampling.
     */
    struct TextureBinding {
        std::string name; ///< Name of the sampler uniform in the shader.
        std::shared_ptr<Texture> texture; ///< Shared pointer to the texture resource.
    };

    /**
     * @brief Parameters for constructing a @ref ShaderMaterial object.
     */
    struct Parameters {
        std::string vertex_shader; ///< Vertex shader code.
        std::string fragment_shader; ///< Fragment shader code.
        UniformList uniforms; ///< Initial uniform values.
        std::vector<TextureBinding> textures; ///< Initial texture bindings.
    };

    /**
     * @brief Constructs a shader material from custom GLSL source strings.
     *
     * @param params @ref ShaderMaterial::Parameters "Initialization parameters"
     * defining the shader sources and initial uniform values.
     */
    explicit ShaderMaterial(Parameters params);

    /**
     * @brief Creates a shared instance of @ref ShaderMaterial.
     *
     * @param params @ref ShaderMaterial::Parameters "Initialization parameters"
     * defining the shader sources and initial uniform values.
     */
    [[nodiscard]] static auto Create(Parameters params) -> std::shared_ptr<ShaderMaterial> {
        return std::make_shared<ShaderMaterial>(std::move(params));
    }

    /**
     * @brief Sets or updates a uniform value by name.
     *
     * Associates a uniform variable in the shader program with the given value.
     * If a uniform with the specified name exists its value is updated.
     * Otherwise, a new uniform entry is created.
     *
     * Uniform values are uploaded to the GPU the next time the material is bound
     * and rendered. The uniform name must exactly match the name declared in the
     * GLSL shader source.
     *
     * @param name Name of the uniform variable as declared in the shader.
     * @param value Value to assign to the uniform.
     */
    auto SetUniform(std::string_view name, UniformValue value) -> void;

    /**
     * @brief Assigns a texture to a shader sampler by name.
     *
     * Binds a texture resource to a specific sampler uniform. If a texture
     * binding with the specified name already exists, it is replaced.
     * Otherwise, a new binding is added to the material.
     *
     * Textures are bound to the appropriate texture units the next time the
     * material is used for rendering. The name must match the sampler2D
     * (or other sampler type) declared in the GLSL shader source.
     *
     * @param name Name of the sampler variable as declared in the shader.
     * @param texture The texture instance to bind.
     */
    auto SetTexture(std::string_view name, std::shared_ptr<Texture> texture) -> void;

    /**
     * @brief Identifies this material as
     * @ref Material::Type "Material::Type::ShaderMaterial".
     */
    auto GetType() const -> Type override {
        return Material::Type::ShaderMaterial;
    }

private:
    /// @cond INTERNAL
    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view v) const noexcept {
            return std::hash<std::string_view>{}(v);
        }
    };

    struct StringEq {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const noexcept {
            return a == b;
        }
    };
    /// @endcond

    friend class Renderer;
    friend class ProgramAttributes;

    std::string vertex_shader_;
    std::string fragment_shader_;

    size_t shader_material_id_;

    std::unordered_map<
        std::string,
        UniformValue,
        StringHash,
        StringEq
    > uniforms_;

    std::vector<TextureBinding> textures_;
};

}