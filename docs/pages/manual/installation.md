# Installation

This page covers how to add VGLX to your project, how to install it system-wide if you prefer, and how to build the engine itself. The goal is to keep the setup simple. All of VGLX's dependencies are included in its repository and you do not need anything beyond a C++ compiler, CMake and an OpenGL driver.

## Requirements

VGLX builds with a C++23-capable toolchain, [CMake](https://cmake.org/) 3.25 or newer, and an OpenGL 4.1+ context. It is regularly tested on the major platforms:

<div class="system-list">

- ![Ubuntu](https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/16x16/UBT.png) Ubuntu 24.04 (GCC 11.3.0)
- ![macOS](https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/16x16/MAC.png) macOS 14 (Clang 15.0.0)
- ![Windows](https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/16x16/WIN.png) Windows 10 and MSVC 19.44

</div>

#### Dependencies

VGLX vendors all of its dependencies directly inside the repository. Nothing is downloaded at build time and no external package managers are required. The runtime pieces are:

| Dependency                                | Version | Location       | Description                                                                         |
| ----------------------------------------- | ------- | -------------- | ----------------------------------------------------------------------------------- |
| [Glad](https://glad.dav1d.de/)            | 0.1.36  | `vendor/glad`  | OpenGL function loader generated from [glad.dav1d.de](https://glad.dav1d.de/).      |
| [GLFW](https://glfw.org/)                 | 3.5.0   | `vendor/glfw`  | Cross-platform window/input/context management (with minor internal modifications). |
| [ImGui](https://github.com/ocornut/imgui) | 1.92.1  | `vendor/imgui` | **Optional** immediate-mode UI library for in-engine tools and examples.            |

Each dependency includes its license inside the `vendor/` directory.

## Adding VGLX to Your Project

The recommended way to use VGLX is to let CMake pull it into your project with `FetchContent`. Nothing needs to be installed on your system: the first configure downloads the pinned release and builds it alongside your application.

```cmake
include(FetchContent)

FetchContent_Declare(
    vglx
    GIT_REPOSITORY https://github.com/shlomnissan/vglx.git
    GIT_TAG v0.4.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(vglx)

target_link_libraries(MyApp PRIVATE vglx::vglx)
```

The `vglx::vglx` target supplies the include paths and VGLX's own dependencies, including the bundled GLFW and glad libraries for the OpenGL backend. Use this target instead of listing archive files and platform libraries yourself.

A few things to know about this setup:

- Your machine needs CMake 3.25 or newer regardless of the minimum version your own project declares because VGLX's build files run as part of your configure.
- When VGLX is built this way it compiles only the library. Its examples, tests and ImGui integration are off by default. To enable ImGui, set `VGLX_BUILD_IMGUI` before `FetchContent_MakeAvailable`:

  ```cmake
  set(VGLX_BUILD_IMGUI ON)
  ```

The quickest way to start is cloning [the starter template](https://github.com/shlomnissan/vglx-starter) which includes this setup:

```bash
 git clone https://github.com/shlomnissan/vglx-starter.git
```

## Installing VGLX System-Wide

If you work on several VGLX projects and would rather not build the engine in each of them, you can install it once and link against the installed copy. The repository includes an `install` preset that configures a release build of the library alone:

```bash
# clone the repository
git clone https://github.com/shlomnissan/vglx.git
cd vglx

# configure and build the library
cmake --preset install
cmake --build out/install --config Release

# install it
cmake --install out/install --config Release --prefix /path/to/vglx
```

Replace `/path/to/vglx` with your installation directory or omit `--prefix` to install to the platform default, which may require administrator privileges. On Windows, the Visual Studio generator builds every configuration from the same directory, so run the build and install commands again with `--config Debug` to make both configurations available.

In your project, locate the installed copy with `find_package`:

```cmake
find_package(vglx CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE vglx::vglx)
```

If the installation directory is outside CMake's standard search locations, pass `-DCMAKE_PREFIX_PATH=/path/to/vglx` when configuring your project. CMake picks the correct build configuration (Debug or Release) based on your project settings.

#### Shared Library Builds

VGLX is built as a static library by default so no separate library needs to accompany your application. If you opted into a shared build with `-DBUILD_SHARED_LIBS=ON` copy the VGLX DLL next to your application on Windows. You can automate this with:

```cmake
if(WIN32)
  add_custom_command(TARGET MyApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:vglx::vglx>
    $<TARGET_FILE_DIR:MyApp>
  )
endif()
```

## Build From Source

To work on the engine itself, use the development preset. The project includes several presets that streamline the process:

- `development` – Debug build with examples, tests, and ImGui enabled
- `development-release` – The same with optimizations, for measuring performance
- `install` – Release build of the library alone, for installation
- `benchmark` – Release build with microbenchmarks enabled

```bash [bash]
# clone the repository
git clone https://github.com/shlomnissan/vglx.git
cd vglx

# configure with a preset
cmake --preset development

# build the engine
cmake --build build/development --config Debug
```

#### Configuration Options

VGLX includes optional components. You can enable or disable them using CMake flags:

| Option                | Description                                                                 |
| --------------------- | --------------------------------------------------------------------------- |
| `BUILD_SHARED_LIBS`   | Build a shared library (default: `OFF`).                                    |
| `VGLX_BUILD_DOCS`     | Build Doxygen documentation.                                                |
| `VGLX_BUILD_EXAMPLES` | Build example applications.                                                 |
| `VGLX_BUILD_IMGUI`    | Enable ImGui support for debug UI/tools.                                    |
| `VGLX_BUILD_TESTS`    | Build unit tests.                                                           |
| `VGLX_INSTALL`        | Generate install rules and the CMake package. |

Examples, tests and ImGui are enabled by default only when VGLX is the top-level project.

#### Verifying Build

If examples are enabled (as in the `development` preset) an `example_<scene>` executable is built for each scene in `examples/scenes`. Run an example to check rendering and input.

## Getting Help

If you run into issues, please [open an issue on GitHub](https://github.com/shlomnissan/vglx/issues). If possible include:

```text
- Your OS and compiler version
- CMake command you ran
- CMake or compiler logs
```
