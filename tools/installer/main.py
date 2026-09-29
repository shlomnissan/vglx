from __future__ import annotations

import sys

from pathlib import Path

from .cmake import get_cmake_version, cmake_configure, build_and_install
from .prompts import ask_choice, ask_yes_no
from .types import ConfigurationOptions
from .helpers import (
    make_error,
    make_info,
    make_warning,
    get_os,
    split_version,
    default_install_prefix
)

MIN_CMAKE_VERSION = "3.25.0"

def configuration_options(os_name: str):
    default_prefix = default_install_prefix(os_name)
    install_path_choice = ask_choice(
        "\nInstallation path:",
        [f"Default ({default_prefix})", "Custom path"],
    )

    is_default_prefix = install_path_choice.startswith("Default")
    install_prefix = default_prefix
    if not is_default_prefix:
        custom = input(
            f"Enter installation prefix [{default_prefix}]: "
        ).strip()
        install_prefix = Path(custom) if custom else default_prefix

    build_shared = ask_choice(
        "\nLibrary target:",
        ["Static library (recommended)", "Shared library"]
    ).startswith("Shared")

    print("\nAdditional components to install with VGLX:")
    build_imgui = ask_yes_no("  *) ImGui: minimal UI library for tools and examples")

    components = []
    if build_imgui:
        components.append("imgui")
    if not components:
        components.append("none")

    print("\nSummary:")
    print(f"  OS: {os_name}")
    print(f"  Install prefix: {install_prefix}")
    print(f"  Library type: {'shared' if build_shared else 'static'}")
    print(f"  Components: {', '.join(components)}")

    if is_default_prefix:
        print()
        make_warning("Default installation path may require admin/sudo privileges")

    if not ask_yes_no("\nProceed with installation?", default_yes=True):
        make_info("Installation cancelled by user.\n")
        sys.exit(0)

    return ConfigurationOptions(
        os_name = os_name,
        install_prefix = install_prefix,
        build_shared = build_shared,
        build_imgui = build_imgui,
    )

def main():
    print("\nVGLX Installer\n")

    root_dir = Path(__file__).resolve().parents[2]
    if not (root_dir / "CMakeLists.txt").exists():
        make_error(
            f"Could not find CMakeLists.txt in {root_dir}.",
            "Please run this installer from within the VGLX repository."
        )

    make_info("Checking Operating System")
    os_name = get_os()
    if os_name == "Unknown":
        make_error(
            "Unkown operating system.",
            "VGLX currently supports Windows, macOS and Linux."
        )

    make_info("Checking CMake")
    cmake_version = get_cmake_version()
    if split_version(cmake_version) < split_version(MIN_CMAKE_VERSION):
       make_error(
            f"CMake {MIN_CMAKE_VERSION} or newer is required.",
            f"Found version {cmake_version}"
        )

    config = configuration_options(os_name)

    print()
    make_info("Configuring CMake")
    cmake_configure(root_dir, config, "Release")
    if config.os_name == "Windows":
        cmake_configure(root_dir, config, "Debug")

    print()
    make_info("Building and Installing VGLX")
    build_and_install(root_dir, "Release")
    if config.os_name == "Windows":
        build_and_install(root_dir, "Debug")

    print()
    make_info("✅ Installation complete\n")


if __name__ == "__main__":
    main()