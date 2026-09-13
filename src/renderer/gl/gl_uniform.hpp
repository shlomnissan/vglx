/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/math/color.hpp"
#include "vglx/math/matrix3.hpp"
#include "vglx/math/matrix4.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/math/vector3.hpp"
#include "vglx/math/vector4.hpp"

#include <string>

#include <glad/glad.h>

namespace vglx {

enum class UniformType {
    Bool,
    Float,
    Int,
    Matrix3,
    Matrix4,
    Sampler,
    Vector2,
    Vector3,
    Vector4,
    Unsupported
};

enum class Uniform {
    AOIntensity,
    AOMap,
    AlbedoMap,
    AlphaMap,
    AlphaTest,
    AmbientLight,
    Anchor,
    BrdfLut,
    Color,
    EmissiveColor,
    EmissiveIntensity,
    EmissiveMap,
    EnvironmentIntensity,
    EnvironmentMap,
    FogColor,
    FogDensity,
    FogFar,
    FogNear,
    FogType,
    IrradianceMap,
    MaterialColor,
    MaterialDiffuseColor,
    MaterialMetallic,
    MaterialRoughness,
    MaterialShininess,
    MaterialSpecularColor,
    MetallicMap,
    Model,
    NormalIntensity,
    NormalMap,
    Opacity,
    PointShadowMaps,
    PrefilteredMap,
    PrefilteredMaxLod,
    ReceiveShadow,
    Reflectivity,
    Rotation,
    RoughnessMap,
    ShadowMaps2D,
    SpecularMap,
    TextureMap,
    TextureTransform,
    KnownUniformsLength,
};

constexpr auto get_uniform_loc(std::string_view str) {
    using enum Uniform;
    if (str == "u_AOIntensity") return static_cast<int>(AOIntensity);
    if (str == "u_AOMap") return static_cast<int>(AOMap);
    if (str == "u_AlbedoMap") return static_cast<int>(AlbedoMap);
    if (str == "u_AlphaMap") return static_cast<int>(AlphaMap);
    if (str == "u_AlphaTest") return static_cast<int>(AlphaTest);
    if (str == "u_AmbientLight") return static_cast<int>(AmbientLight);
    if (str == "u_Anchor") return static_cast<int>(Anchor);
    if (str == "u_BrdfLut") return static_cast<int>(BrdfLut);
    if (str == "u_Color") return static_cast<int>(Color);
    if (str == "u_EmissiveColor") return static_cast<int>(EmissiveColor);
    if (str == "u_EmissiveIntensity") return static_cast<int>(EmissiveIntensity);
    if (str == "u_EmissiveMap") return static_cast<int>(EmissiveMap);
    if (str == "u_EnvironmentIntensity") return static_cast<int>(EnvironmentIntensity);
    if (str == "u_EnvironmentMap") return static_cast<int>(EnvironmentMap);
    if (str == "u_Fog.Color") return static_cast<int>(FogColor);
    if (str == "u_Fog.Density") return static_cast<int>(FogDensity);
    if (str == "u_Fog.Far") return static_cast<int>(FogFar);
    if (str == "u_Fog.Near") return static_cast<int>(FogNear);
    if (str == "u_Fog.Type") return static_cast<int>(FogType);
    if (str == "u_IrradianceMap") return static_cast<int>(IrradianceMap);
    if (str == "u_Material.Color") return static_cast<int>(MaterialColor);
    if (str == "u_Material.DiffuseColor") return static_cast<int>(MaterialDiffuseColor);
    if (str == "u_Material.Metallic") return static_cast<int>(MaterialMetallic);
    if (str == "u_Material.Roughness") return static_cast<int>(MaterialRoughness);
    if (str == "u_Material.Shininess") return static_cast<int>(MaterialShininess);
    if (str == "u_Material.SpecularColor") return static_cast<int>(MaterialSpecularColor);
    if (str == "u_MetallicMap") return static_cast<int>(MetallicMap);
    if (str == "u_Model") return static_cast<int>(Model);
    if (str == "u_NormalIntensity") return static_cast<int>(NormalIntensity);
    if (str == "u_NormalMap") return static_cast<int>(NormalMap);
    if (str == "u_Opacity") return static_cast<int>(Opacity);
    if (str == "u_PointShadowMaps") return static_cast<int>(PointShadowMaps);
    if (str == "u_PrefilteredMap") return static_cast<int>(PrefilteredMap);
    if (str == "u_PrefilteredMaxLod") return static_cast<int>(PrefilteredMaxLod);
    if (str == "u_Reflectivity") return static_cast<int>(Reflectivity);
    if (str == "u_ReceiveShadow") return static_cast<int>(ReceiveShadow);
    if (str == "u_Rotation") return static_cast<int>(Rotation);
    if (str == "u_RoughnessMap") return static_cast<int>(RoughnessMap);
    if (str == "u_ShadowMaps2D") return static_cast<int>(ShadowMaps2D);
    if (str == "u_SpecularMap") return static_cast<int>(SpecularMap);
    if (str == "u_TextureMap") return static_cast<int>(TextureMap);
    if (str == "u_TextureTransform") return static_cast<int>(TextureTransform);
    return -1;
}

class GLUniform {
public:
    GLUniform(std::string_view name, GLint location, GLenum type);

    auto SetValue(const void* value) -> void;

    auto UploadIfNeeded() -> void;

private:
    std::string name_;

    GLint location_ {-1};

    UniformType type_;

    bool needs_upload_ {true};

    union {
        GLboolean b;
        GLfloat f;
        GLint i;
        Matrix3 m3;
        Matrix4 m4;
        Vector2 v2;
        Vector3 v3;
        Vector4 v4;
    } data_ {};
};

}
