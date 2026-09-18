/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/canvas/sprite.hpp"

#include "vglx/geometries/buffer_attribute.hpp"
#include "vglx/textures/image.hpp"

#include <utility>

namespace vglx {

Sprite::Sprite(std::shared_ptr<Texture2D> texture) : texture(std::move(texture)) {}

auto Sprite::GetSize() const -> Vector2 {
    if (region.has_value()) {
        return region->Size();
    }

    if (texture == nullptr || texture->image == nullptr) {
        return Vector2 {0.0f, 0.0f};
    }

    return Vector2 {
        static_cast<float>(texture->image->width),
        static_cast<float>(texture->image->height)
    };
}

auto Sprite::SharedGeometry() -> std::shared_ptr<Geometry>& {
    static auto geometry = std::shared_ptr<Geometry> {};

    if (geometry == nullptr || geometry->Disposed()) {
        auto g = Geometry::Create();

        g->AddAttribute(BufferAttribute::Create({
            .name = BufferAttribute::kPosition,
            .format = BufferAttribute::Format::Float32x3,
            .rate = BufferAttribute::Rate::Vertex
        }, {
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f
        }));

        g->AddAttribute(BufferAttribute::Create({
            .name = BufferAttribute::kTexCoord,
            .format = BufferAttribute::Format::Float32x2,
            .rate = BufferAttribute::Rate::Vertex
        }, {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        }));

        g->SetIndices({0, 2, 1, 0, 3, 2});

        geometry = std::move(g);
    }

    return geometry;
}

}
