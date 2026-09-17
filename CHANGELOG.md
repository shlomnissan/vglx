# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project adheres to [Semantic Versioning](https://semver.org). While the engine is pre-`1.0.0` minor releases may include breaking API changes and patch releases are limited to fixes.

## [Unreleased]

### Added

- `const Get()` overloads on transforms that compute the matrix without updating the cache

## [0.2.0] - 2026-09-16

### Added

- Shadow map support for custom shader materials
- Ambient light uniform for custom shader materials
- Frame uniform block exposing resolution, content scale, and elapsed time
- Protected default constructor for `Mesh` subclasses
- `Geometry::ApplyTransform` for baking a transform into vertex data
- Gamepad input through `GamepadEvent` and `Node::OnGamepadEvent`

### Changed

- `Renderer::SetViewport` now takes the content scale

### Fixed

- Crash when rendering a node with a null material

## [0.1.0] - 2026-09-01

### Added

- Dynamic buffer attributes for geometries and instanced meshes
- Runtime generation of normals and tangents for geometries
- Explicit draw-order override for renderables
- Optional manual depth and color buffer clears
- Orientation selection for plane geometry
- Instanced mesh example

### Changed

- Texture units are now allocated dynamically
- Renamed the `Sprite` renderable to `Billboard` (breaking)
- Shader attributes are now inlined per material
