# Third-party libraries

This directory contains third-party libraries bundled directly with VGLX.
Each subdirectory holds one dependency at a pinned version. Entries below are
listed alphabetically and must match their respective folder names.


## glad

- Upstream: https://github.com/Dav1dde/glad
- Version: 0.1.36
- License: MIT

Files generated from upstream web instance:

- `glad/glad.h`
- `KHR/khrplatform.h`
- `glad.c`
- `LICENSE.txt`


## glfw

- Upstream: https://github.com/glfw/glfw
- Version: 3.5.0
- License: zlib/libpng


## imgui

- Upstream: https://github.com/ocornut/imgui
- Version: 1.92.1
- License: MIT


## misc

Collection of single-file libraries used in VGLX components.

- `cgltf.hpp`
  * Upstream: https://github.com/jkuhlmann/cgltf
  * Version: 1.15
  * License: MIT

- `stb_image.hpp`
  * Upstream: https://github.com/nothings/stb
  * Version: 2.30
  * License: Public Domain or MIT

- `tiny_obj_loader.hpp`
  * Upstream: https://github.com/tinyobjloader/tinyobjloader
  * Version: 2.0.0
  * License: MIT


## vma

- Upstream: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
- Version: 3.4.0
- License: MIT

Single-header Vulkan memory allocator. Only `include/vk_mem_alloc.h` is
vendored. Used only by the experimental Vulkan backend.


## volk

- Upstream: https://github.com/zeux/volk
- Version: 1.4.350
- License: MIT

Meta-loader that loads the Vulkan loader at runtime. Used only by the
experimental Vulkan backend, where it replaces linking against the
Vulkan SDK.


## vulkan-headers

- Upstream: https://github.com/KhronosGroup/Vulkan-Headers
- Version: v1.4.350
- License: Apache-2.0 / MIT

Only the C headers under `include/` are vendored. The C++ bindings and the
registry are omitted. Used only by the experimental Vulkan backend.
