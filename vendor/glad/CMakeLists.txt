set(GLAD_SOURCES
    "glad.c"
)

set(GLAD_HEADERS
    "glad/glad.h"
    "KHR/khrplatform.h"
)

add_library(glad STATIC ${GLAD_SOURCES} ${GLAD_HEADERS})

target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(glad PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if (VGLX_INSTALL)
    install(TARGETS glad EXPORT vglx ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>)
endif()