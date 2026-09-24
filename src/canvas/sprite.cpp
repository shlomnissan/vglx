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
    return region.has_value() ? region->Size() : TextureSize();
}

auto Sprite::GetGeometryTransform() const -> Matrix3 {
    const auto size = GetSize();
    const auto origin = Vector2 {-anchor.x * size.x, -anchor.y * size.y};

    return Matrix3 {
        size.x, 0.0f, origin.x,
        0.0f, size.y, origin.y,
        0.0f, 0.0f, 1.0f
    };
}

auto Sprite::GetTextureTransform() const -> Matrix3 {
    const auto texture_size = TextureSize();
    if (texture_size.x <= 0.0f || texture_size.y <= 0.0f) {
        return Matrix3 {1.0f};
    }

    const auto r = region.value_or(Rect {0.0f, 0.0f, texture_size.x, texture_size.y});

    // Images are flipped on load so V = 1 is the top of the image. Start at
    // the region's top edge and scale V negatively so it runs downward.
    return Matrix3 {
        r.width / texture_size.x, 0.0f, r.x / texture_size.x,
        0.0f, -r.height / texture_size.y, 1.0f - r.y / texture_size.y,
        0.0f, 0.0f, 1.0f
    };
}

auto Sprite::TextureSize() const -> Vector2 {
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
