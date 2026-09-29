set(GLFW_SOURCES
    "context.c"
    "egl_context.c"
    "init.c"
    "input.c"
    "monitor.c"
    "null_init.c"
    "null_joystick.c"
    "null_monitor.c"
    "null_window.c"
    "osmesa_context.c"
    "platform.c"
    "vulkan.c"
    "window.c"
)

set(GLFW_HEADERS
    "${GLFW_SOURCE_DIR}/include/GLFW/glfw3.h"
    "${GLFW_SOURCE_DIR}/include/GLFW/glfw3native.h"
    "internal.h"
    "mappings.h"
    "null_joystick.h"
    "null_platform.h"
    "platform.h"
)

add_library(glfw STATIC ${GLFW_SOURCES} ${GLFW_HEADERS})

target_include_directories(glfw PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if (VGLX_INSTALL)
    install(TARGETS glfw EXPORT vglx ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>)
endif()

# The time, thread and module code is shared between all backends on a given OS,
# including the null backend, which still needs those bits to be functional
if (APPLE)
    target_sources(glfw PRIVATE
        "cocoa_time.h"
        "cocoa_time.c"
        "posix_thread.h"
        "posix_module.c"
        "posix_thread.c"
    )
elseif (WIN32)
    target_sources(glfw PRIVATE
        "win32_time.h"
        "win32_thread.h"
        "win32_module.c"
        "win32_time.c"
        "win32_thread.c"
    )
else()
    target_sources(glfw PRIVATE
        "posix_time.h"
        "posix_thread.h"
        "posix_module.c"
        "posix_time.c"
        "posix_thread.c"
    )
endif()

add_custom_target(update_mappings
    COMMAND "${CMAKE_COMMAND}" -P "${GLFW_SOURCE_DIR}/CMake/GenerateMappings.cmake" "mappings.h.in" "mappings.h"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Updating gamepad mappings from upstream repository"
    SOURCES "mappings.h.in" "${GLFW_SOURCE_DIR}/CMake/GenerateMappings.cmake"
    VERBATIM
)

set_target_properties(update_mappings PROPERTIES FOLDER "GLFW3")

if (GLFW_BUILD_COCOA)
    enable_language(OBJC)
    target_compile_definitions(glfw PRIVATE _GLFW_COCOA)
    target_sources(glfw PRIVATE
        "cocoa_platform.h"
        "cocoa_joystick.h"
        "cocoa_init.m"
        "cocoa_joystick.m"
        "cocoa_monitor.m"
        "cocoa_window.m"
        "nsgl_context.m"
    )
endif()

if (GLFW_BUILD_WIN32)
    target_compile_definitions(glfw PRIVATE _GLFW_WIN32)
    target_sources(glfw PRIVATE
        "win32_platform.h"
        "win32_joystick.h"
        "win32_init.c"
        "win32_joystick.c"
        "win32_monitor.c"
        "win32_window.c"
        "wgl_context.c"
    )
endif()

if (GLFW_BUILD_X11)
    target_compile_definitions(glfw PRIVATE _GLFW_X11)
    target_sources(glfw PRIVATE
        "x11_platform.h"
        "xkb_unicode.h"
        "x11_init.c"
        "x11_monitor.c"
        "x11_window.c"
        "xkb_unicode.c"
        "glx_context.c"
    )
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_sources(glfw PRIVATE "linux_joystick.h" "linux_joystick.c")
    endif()
    target_sources(glfw PRIVATE "posix_poll.h" "posix_poll.c")
endif()

set_target_properties(glfw PROPERTIES
    OUTPUT_NAME glfw
    VERSION ${GLFW_VERSION_MAJOR}.${GLFW_VERSION_MINOR}
    SOVERSION ${GLFW_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 99
    C_EXTENSIONS OFF
    DEFINE_SYMBOL _GLFW_BUILD_DLL
    FOLDER "GLFW3"
)

target_include_directories(glfw PUBLIC
    "$<BUILD_INTERFACE:${GLFW_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_include_directories(glfw PRIVATE
    "${GLFW_SOURCE_DIR}/src"
    "${GLFW_BINARY_DIR}/src"
)

find_package(Threads REQUIRED)
target_link_libraries(glfw PRIVATE Threads::Threads)

if (GLFW_BUILD_WIN32)
    list(APPEND glfw_PKG_LIBS "-lgdi32")
endif()

if (GLFW_BUILD_COCOA)
    target_link_libraries(glfw PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework QuartzCore"
    )
    set(glfw_PKG_DEPS "")
    set(glfw_PKG_LIBS "-framework Cocoa -framework IOKit -framework CoreFoundation -framework QuartzCore")
endif()

if (GLFW_BUILD_X11)
    find_package(X11 REQUIRED)
    target_include_directories(glfw PRIVATE "${X11_X11_INCLUDE_PATH}")

    # Check for XRandR (modern resolution switching and gamma control)
    if (NOT X11_Xrandr_INCLUDE_PATH)
        message(FATAL_ERROR "RandR headers not found; install libxrandr development package")
    endif()
    target_include_directories(glfw PRIVATE "${X11_Xrandr_INCLUDE_PATH}")

    # Check for Xinerama (legacy multi-monitor support)
    if (NOT X11_Xinerama_INCLUDE_PATH)
        message(FATAL_ERROR "Xinerama headers not found; install libxinerama development package")
    endif()
    target_include_directories(glfw PRIVATE "${X11_Xinerama_INCLUDE_PATH}")

    # Check for Xkb (X keyboard extension)
    if (NOT X11_Xkb_INCLUDE_PATH)
        message(FATAL_ERROR "XKB headers not found; install X11 development package")
    endif()
    target_include_directories(glfw PRIVATE "${X11_Xkb_INCLUDE_PATH}")

    # Check for Xcursor (cursor creation from RGBA images)
    if (NOT X11_Xcursor_INCLUDE_PATH)
        message(FATAL_ERROR "Xcursor headers not found; install libxcursor development package")
    endif()
    target_include_directories(glfw PRIVATE "${X11_Xcursor_INCLUDE_PATH}")

    # Check for XInput (modern HID input)
    if (NOT X11_Xi_INCLUDE_PATH)
        message(FATAL_ERROR "XInput headers not found; install libxi development package")
    endif()
    target_include_directories(glfw PRIVATE "${X11_Xi_INCLUDE_PATH}")

    # Check for X Shape (custom window input shape)
    if (NOT X11_Xshape_INCLUDE_PATH)
        message(FATAL_ERROR "X Shape headers not found; install libxext development package")
    endif()
    target_include_directories(glfw PRIVATE "${X11_Xshape_INCLUDE_PATH}")
endif()

if (UNIX AND NOT APPLE)
    find_library(RT_LIBRARY rt)
    mark_as_advanced(RT_LIBRARY)
    if (RT_LIBRARY)
        target_link_libraries(glfw PRIVATE "${RT_LIBRARY}")
        list(APPEND glfw_PKG_LIBS "-lrt")
    endif()

    find_library(MATH_LIBRARY m)
    mark_as_advanced(MATH_LIBRARY)
    if (MATH_LIBRARY)
        target_link_libraries(glfw PRIVATE "${MATH_LIBRARY}")
        list(APPEND glfw_PKG_LIBS "-lm")
    endif()

    if (CMAKE_DL_LIBS)
        target_link_libraries(glfw PRIVATE "${CMAKE_DL_LIBS}")
        list(APPEND glfw_PKG_LIBS "-l${CMAKE_DL_LIBS}")
    endif()
endif()

if (WIN32)
    if (GLFW_USE_HYBRID_HPG)
        target_compile_definitions(glfw PRIVATE _GLFW_USE_HYBRID_HPG)
    endif()
endif()

# Enable a reasonable set of warnings
# NOTE: The order matters here, Clang-CL matches both MSVC and Clang
if (MSVC)
    target_compile_options(glfw PRIVATE "/W3")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_C_COMPILER_ID STREQUAL "Clang" OR
        CMAKE_C_COMPILER_ID STREQUAL "AppleClang")

    target_compile_options(glfw PRIVATE "-Wall")
endif()

if (GLFW_BUILD_WIN32)
    target_compile_definitions(glfw PRIVATE UNICODE _UNICODE)
endif()

# HACK: When building on MinGW, WINVER and UNICODE need to be defined before
# the inclusion of stddef.h (by glfw3.h), which is itself included before
# win32_platform.h.  We define them here until a saner solution can be found
# NOTE: MinGW-w64 and Visual C++ do /not/ need this hack.
if (MINGW)
    target_compile_definitions(glfw PRIVATE WINVER=0x0501)
endif()

# Workaround for legacy MinGW not providing XInput and DirectInput
if (MINGW)
    include(CheckIncludeFile)
    check_include_file("dinput.h" DINPUT_H_FOUND)
    check_include_file("xinput.h" XINPUT_H_FOUND)
    if (NOT DINPUT_H_FOUND OR NOT XINPUT_H_FOUND)
        target_include_directories(glfw PRIVATE "${GLFW_SOURCE_DIR}/deps/mingw")
    endif()
endif()

# Workaround for the MS CRT deprecating parts of the standard library
if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
    target_compile_definitions(glfw PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Workaround for -std=c99 on Linux disabling _DEFAULT_SOURCE (POSIX 2008 and more)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(glfw PRIVATE _DEFAULT_SOURCE)
endif()