# Custom Shaders

In VGLX, shaders are attached to materials and materials are paired with geometry. A [Mesh](/reference/scene/mesh) is the fundamental renderable unit in the engine: it combines geometry which describes what is drawn, with a material which describes how that geometry is shaded.

The engine provides several built-in materials each backed by its own shader program. These materials are typically sufficient and allow you to build scenes without writing any shader code at all.

When you need more control over how geometry is transformed and shaded you use the [Shader Material](/reference/materials/shader_material). This material lets you supply your own vertex and fragment shader code giving you full control over vertex transformation and fragment shading while still integrating cleanly with the rest of the engine.

VGLX uses [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) for shader programs. Writing custom shaders assumes familiarity with shader programming concepts such as vertex and fragment stages, attributes, uniforms, and varyings. This guide does not teach GLSL. It focuses on how custom shaders fit in.

This guide demonstrates how to write a minimal custom shader program, attach it to a shader material, and use it for rendering. It also explains how geometry attributes are matched to shader declarations, which built-in uniforms the engine provides, and how custom uniforms are defined and updated from the application.

You can do a lot with VGLX without writing custom shaders, but if you want full flexibility over how your geometry is rendered custom shaders are the extension point.

## Shader Materials

A [Shader Material](/reference/materials/shader_material) is a material backed by user-defined shaders. Unlike built-in materials, shader materials do not come with a predefined shader program. You provide code for both the vertex and fragment shaders when constructing the material.

The structure around custom shaders is intentionally light. Your shaders are self-contained GLSL programs: you declare the attributes, uniforms, and varyings you use, and the engine supplies their values by name.

We can start with a simple scene that renders a rotating cube. The scene uses a built-in [Unlit Material](/reference/materials/unlit_material) that colors the cube with a single constant color:

```cpp
struct MyScene : public vglx::Scene {
    vglx::Mesh* mesh {nullptr};

    MyScene(vglx::Camera* camera) {
        camera->transform.Translate({0.0f, 0.0f, 2.5f});

        mesh = Add(vglx::Mesh::Create(
            vglx::BoxGeometry::Create(),
            vglx::UnlitMaterial::Create({.color = 0xFF00FFu})
        ));
    }

    auto OnUpdate(float delta) -> void override {
        const auto speed = vglx::math::pi_over_2;
        mesh->transform.Rotate(vglx::Vector3::UnitX(), speed * delta);
        mesh->transform.Rotate(vglx::Vector3::UnitY(), speed * delta);
    }
};
```

To demonstrate how to use shader materials, we replace the built-in material with a custom shader that implements the same behavior.

Let's define the vertex and fragment shader source code as raw string literals, connect them to a shader material, and attach the material to the mesh:

```cpp
constexpr auto vertex_shader = R"(
#version 410 core
#pragma inject_attributes

in vec3 a_Position;

uniform mat4 u_Model;

layout(std140) uniform ub_Camera {
    mat4 u_Projection;
    mat4 u_View;
};

void main() {
    gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0);
}
)";

constexpr auto fragment_shader = R"(
#version 410 core
#pragma inject_attributes

layout (location = 0) out vec4 o_FragColor;

uniform vec3 color;

void main() {
    o_FragColor = vec4(color, 1.0);
}
)";

vglx::Mesh* mesh {nullptr};

std::shared_ptr<vglx::ShaderMaterial> material {nullptr};

MyScene(vglx::Camera* camera) {
    material = vglx::ShaderMaterial::Create({
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .uniforms = {{"color", vglx::Color {0xFF00FFu}}}
    });

    mesh = Add(vglx::Mesh::Create(vglx::BoxGeometry::Create(), material));
}
```

Running the application now produces the same visual result as before. The difference is that the cube is now rendered using a custom shader program rather than a built-in material.

A few notes about the shader program we created:
- The shader material factory expects valid GLSL source code for both shader stages.
- VGLX targets GLSL 4.10 core so the version directive should always be `410 core`.
- Custom uniforms should be provided at construction time as a name/value map.
- The `#pragma inject_attributes` directive injects preprocessor definitions based on the material and program configuration. It is required in both shader stages.

This minimal example implements a complete custom shader program. While the shader itself is simple, the same structure scales to complex techniques.

## Shader Interface

Shader programs interact with the engine through a small set of conventions: attribute names that match the geometry's buffer attributes, built-in uniforms populated by name, and preprocessor definitions injected through the `#pragma inject_attributes` directive.

#### Vertex Attributes

Vertex attributes describe per-vertex or per-instance data supplied by the geometry. Attribute declarations are matched to [buffer attributes](/reference/geometries/buffer_attribute) by name so any attribute the geometry provides, including custom attributes, is available under its buffer attribute name.

These are the names used by geometry generators and asset loaders:

| Name                  | Type   | Description                     |
| --------------------- | ------ | ------------------------------- |
| `a_Position`          | `vec3` | Vertex position in local space  |
| `a_Normal`            | `vec3` | Vertex normal in local space    |
| `a_TexCoord`          | `vec2` | Primary texture coordinates     |
| `a_Tangent`           | `vec4` | Tangent and handedness          |
| `a_Color`             | `vec3` | Per-vertex color attribute      |
| `a_InstanceColor`     | `vec3` | Per-instance color modifier     |
| `a_InstanceTransform` | `mat4` | Per-instance model transform    |

Declare only the attributes your shader uses. An attribute that is declared and used becomes a hard requirement: if the geometry does not provide a matching buffer attribute the mesh will not render and an error is logged.

Instance attributes are only available when the material is used with an [Instanced Mesh](/reference/scene/instanced_mesh).

#### Uniforms

Built-in uniforms provide per-draw or per-frame state supplied by the engine. Declare the ones you use and they are populated automatically:

| Name                 | Type    | Description                                 |
| -------------------- | ------- | ------------------------------------------- |
| `u_Model`            | `mat4`  | World transform                             |
| `u_TextureTransform` | `mat3`  | Texture transform                           |
| `u_Opacity`          | `float` | Alpha factor                                |
| `u_AlphaTest`        | `float` | Alpha cutoff                                |
| `u_Resolution`       | `vec2`  | Framebuffer resolution in pixels            |

The camera matrices are provided through a uniform block. Declaring the block makes both matrices available and the engine binds it automatically:

```glsl
layout(std140) uniform ub_Camera {
    mat4 u_Projection;
    mat4 u_View;
};
```

#### Preprocessor Definitions

The `#pragma inject_attributes` directive injects preprocessor definitions that describe the material and scene configuration, allowing a single shader source to adapt to its rendering context. These are the definitions relevant to shader materials:

| Name                    | Description                                           |
| ----------------------- | ----------------------------------------------------- |
| `USE_INSTANCING`        | The material is rendered by an instanced mesh         |
| `USE_FOG`               | The material has fog enabled and the scene defines it |
| `USE_ALPHA_TEST`        | The material's alpha test threshold is set            |
| `USE_FLAT_SHADED`       | The material is flat shaded                           |
| `USE_FLIP_NORMALS`      | The material renders back or double-sided faces       |
| `USE_VERTEX_COLOR`      | The geometry provides an `a_Color` attribute          |
| `USE_SHADOW_MAPS`       | Shadow maps are enabled and a light casts shadows     |
| `USE_PCF_SHADOWS`       | Shadow maps use percentage-closer filtering           |
| `USE_POINT_SHADOW_MAPS` | A point light casts shadows                           |
| `NUM_LIGHTS`            | Number of active lights in the scene (always defined) |

#### Includes

Two shader includes can be brought into fragment shaders using standard `#include` directives. They are self-contained functions resolved by the engine before compilation:

- `include/fog.incl.glsl` defines the `Fog` uniform struct and `applyFog(inout vec3 color, const in float depth)`. Its contents are gated behind `USE_FOG` so it is safe to include unconditionally. The depth argument is the fragment's view-space depth, which the vertex stage typically provides through a varying.
- `include/lights.incl.glsl` defines the `Light` struct, the `ub_Lights` uniform block, and the `attenuation` and `shadowFactor` helpers used by the built-in lit materials. It must be included inside an `#if NUM_LIGHTS > 0` block. The light data is uploaded by the engine automatically so including the file is all that is needed to iterate the scene's lights. The position argument of `shadowFactor` is the fragment's view-space position, which the vertex stage typically provides through a varying. Note that point light shadows additionally require the `ub_Camera` uniform block to be declared in the fragment stage.

## Custom Uniforms

Custom uniforms are values defined by the application and consumed by shader programs. They must be declared in GLSL and provided through the uniform map. Uniforms are bound by name and the type provided by the application must match the shader.

The following table lists accepted uniform value types and the GLSL types they map to:

| Application Type | GLSL Type | Example                                  |
| ---------------- | --------- | ---------------------------------------- |
| `bool`           | `bool`    | `{true}`                                 |
| `int`            | `int`     | `{1}`                                    |
| `float`          | `float`   | `{1.5f}`                                 |
| `Color`          | `vec3`    | `vglx::Color {0xFF00FFu}`                |
| `Matrix3`        | `mat3`    | `vglx::Matrix3 {1.0f}`                   |
| `Matrix4`        | `mat4`    | `vglx::Matrix4 {1.0f}`                   |
| `Vector2`        | `vec2`    | `vglx::Vector2 {1.0f, 0.0f}`             |
| `Vector3`        | `vec3`    | `vglx::Vector3 {1.0f, 0.0f, 1.0f}`       |
| `Vector4`        | `vec4`    | `vglx::Vector4 {1.0f, 0.0f, 1.0f, 0.0f}` |

Uniform values can be updated after creation using the material's [SetUniform](/reference/materials/shader_material#function-set-uniform-96f6b87f) method. Updates take effect the next time the material is rendered.

## Textures

Shader materials can sample textures like any other material. A texture is associated with a sampler uniform by name, either at construction time through the texture bindings list or later using the material's [SetTexture](/reference/materials/shader_material#function-set-texture-7891f192) method:

```cpp
material = vglx::ShaderMaterial::Create({
    .vertex_shader = vertex_shader,
    .fragment_shader = fragment_shader,
    .textures = {{"u_MyTexture", texture}}
});
```

The sampler is declared in the fragment shader like any other uniform and the engine binds the texture to an available texture unit automatically:

```glsl
uniform sampler2D u_MyTexture;
```

Sampling requires texture coordinates which the geometry provides through the `a_TexCoord` attribute. When a bound texture defines a [UV transform](/reference/textures/texture2d), the engine uploads it through the `u_TextureTransform` uniform.

## Debugging Shaders

Shaders can be difficult to debug. A single mistake may result in a black screen, incorrect colors, or missing geometry. When you run into problems start from the smallest shader that works and add features back incrementally.

For deeper issues the most effective tool is [RenderDoc](https://renderdoc.org/). Capture a frame and inspect the draw call that renders your mesh to verify the full GPU state:

- Ensure the expected vertex and fragment shaders are bound.
- Verify attribute bindings and formats match the shader declarations.
- Inspect uniform values and confirm names and types are correct.
- Check that the correct textures are bound and sampled.
- Confirm the fragment shader writes to the intended render target.

Most shader issues in VGLX come from mismatches between what the shader declares and what the engine provides. When a shader compiles but renders incorrectly RenderDoc is the fastest way to inspect the application state.
