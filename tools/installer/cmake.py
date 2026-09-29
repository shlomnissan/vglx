from __future__ import annotations

import shutil
import subprocess

from pathlib import Path

from .helpers import make_error, run_command
from .types import ConfigurationOptions

def get_cmake_version():
    cmake_path = shutil.which("cmake")
    if not cmake_path:
        make_error(
            "CMake was not found on your PATH.",
            "Please install CMake 3.25 or newer and try again."
        )

    output = subprocess.run(
        [cmake_path, "--version"],
        capture_output = True,
        text = True,
        check = True,
    )

    line = output.stdout.splitlines()[0] if output.stdout else ""
    tokens = line.strip().split()
    return tokens[-1] if tokens else ""

def cmake_configure(
    root_dir: Path,
    config: ConfigurationOptions,
    build_type: str
):
    build_dir = root_dir / build_type.lower()
    build_dir.mkdir(parents=True, exist_ok=True)

    args = [
        "cmake",
        "-S",
        str(root_dir),
        "-B",
        str(build_dir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_INSTALL_PREFIX={config.install_prefix}",
        f"-DBUILD_SHARED_LIBS={'ON' if config.build_shared else 'OFF'}",
        f"-DVGLX_BUILD_IMGUI={'ON' if config.build_imgui else 'OFF'}",
        f"-DVGLX_LOG_LEVEL={1 if build_type == "Release" else 3}",
        "-DVGLX_BUILD_EXAMPLES=OFF",
        "-DVGLX_BUILD_DOCS=OFF",
        "-DVGLX_BUILD_TESTS=OFF"
    ]

    print()
    if run_command(args, cwd = root_dir) != 0:
        print()
        make_error(
            "CMake configuration failed.",
            "Please review the messages above and try again.",
        )

def build_and_install(root_dir: Path, build_type: str):
    build_dir = root_dir / build_type.lower()

    args = [
        "cmake",
        "--build",
        str(build_dir),
        "--config",
        build_type
    ]

    print()
    if run_command(args, cwd = build_dir) != 0:
        print()
        make_error(
            "Build failed.",
            "Please review the compiler output above.",
        )

    install_args = ["cmake", "--install", str(build_dir), "--config", build_type]
    if run_command(install_args, cwd = build_dir) != 0:
        make_error(
            "Install step failed.",
            "Check permissions for the installation prefix and try again.",
        )