# Installation

This page covers how to build VGLX, install it, and use it in your own project. The goal is to keep the setup simple. All dependencies are included in the repository, and you do not need anything system-wide beyond a C++ compiler, CMake and an OpenGL driver.

## Requirements

VGLX builds with a C++23-capable toolchain, [CMake](https://cmake.org/) 3.25 or newer, and an OpenGL 4.1+ context. It is regularly tested on the major platforms:

<div class="system-list">

- ![Ubuntu](https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/16x16/UBT.png) Ubuntu 24.04 (GCC 11.3.0)
- ![macOS](https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/16x16/MAC.png) macOS 14 (Clang 15.0.0)
- ![Windows](https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/16x16/WIN.png) Windows 10 and MSVC 19.44

</div>

#### Dependencies

VGLX vendors all of its dependencies directly inside the repository. Nothing is downloaded at build time, and no external package managers are required. The runtime pieces are:

| Dependency                                | Version | Location       | Description                                                                         |
| ----------------------------------------- | ------- | -------------- | ----------------------------------------------------------------------------------- |
| [Glad](https://glad.dav1d.de/)            | 0.1.36  | `vendor/glad`  | OpenGL function loader generated from [glad.dav1d.de](https://glad.dav1d.de/).      |
| [GLFW](https://glfw.org/)                 | 3.5.0   | `vendor/glfw`  | Cross-platform window/input/context management (with minor internal modifications). |
| [ImGui](https://github.com/ocornut/imgui) | 1.92.1  | `vendor/imgui` | **Optional** immediate-mode UI library for in-engine tools and examples.            |

Each dependency includes its license inside the `vendor/` directory.

## VGLX Installer

The easiest way to install VGLX is using the Python installer included in the repository. It guides you through the process and builds the engine using the right presets for your system.

```bash
# clone the repository
git clone https://github.com/shlomnissan/vglx.git
cd vglx

# run the installer
python3 -m tools.installer.main
```

The installer checks for a working version of CMake, detects your compiler and asks for an installation prefix.

If you encounter issues, see the [Getting Help](#getting-help) section below.

## Creating a New Project

The quickest way to get started is cloning [the starter template](https://github.com/shlomnissan/vglx-starter):

```bash
 git clone https://github.com/shlomnissan/vglx-starter.git
```

It includes a simple application wired to VGLX using CMake. You can also start from scratch with the following setup.

If your project uses CMake, the recommended way to integrate VGLX is through `find_package`:

```cmake
find_package(vglx CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE vglx::vglx)
```

CMake automatically picks the correct build configuration (Debug or Release) based on your project settings. The `vglx::vglx` target also supplies the C++23 requirement and transitive link dependencies, including the bundled GLFW and glad libraries for the OpenGL backend. Use this target instead of manually listing archive files and platform libraries.

With the default static build, no separate VGLX shared library needs to accompany your application. Platform and compiler runtime requirements still apply. If you opted into a shared build, copy the VGLX DLL next to your application on Windows. You can automate this with:

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

The project includes several presets that streamline the process:

- `development` – Development build with examples, tests, and ImGui enabled
- `install-debug` – Debug build for installation, with examples and tests disabled
- `install-release` – Release build for installation, with examples and tests disabled
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

VGLX includes optional build components. You can enable or disable them using standard CMake flags:

| Option                | Description                              |
| --------------------- | ---------------------------------------- |
| `BUILD_SHARED_LIBS`   | Build a shared library (default: `OFF`). |
| `VGLX_BUILD_DOCS`     | Build Doxygen documentation.             |
| `VGLX_BUILD_EXAMPLES` | Build example applications.              |
| `VGLX_BUILD_IMGUI`    | Enable ImGui support for debug UI/tools. |
| `VGLX_BUILD_TESTS`    | Build unit tests.                        |

Builds default to a static library. To configure a shared installation build, use:

```bash
cmake --preset install-release -DBUILD_SHARED_LIBS=ON
```

#### Verifying Build

If examples are enabled (as in the `development` preset), an `example_<scene>` executable is built for each scene in `examples/scenes`. Find them under `build/development/examples`, or its configuration subdirectory when using a generator such as Visual Studio. Run an example to check rendering and input.

#### Manual Installation

If you want full control over the installation process, you can use CMake directly:

```bash
git clone https://github.com/shlomnissan/vglx.git
cd vglx

cmake --preset install-release
cmake --build out/install-release --config Release
cmake --install out/install-release --config Release --prefix /path/to/vglx
```

Replace `/path/to/vglx` with your installation directory. If it is outside CMake's standard search locations, pass `-DCMAKE_PREFIX_PATH=/path/to/vglx` when configuring your application.

On Windows, install both Debug and Release configurations if your application uses both. Repeat the commands with the `install-debug` preset, `out/install-debug` build directory, and `--config Debug`, using the same installation prefix.

## Getting Help

If you run into issues, please [open an issue on GitHub](https://github.com/shlomnissan/vglx/issues). If possible include:

```text
- Your OS and compiler version
- CMake command you ran
- Installer or compiler logs
```

If you discover a fix, you are encouraged to open a PR with updates to this document so the whole community benefits.
