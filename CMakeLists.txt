cmake_minimum_required(VERSION 3.14)

project(InlinePolyContainers
    VERSION 0.2.0
    DESCRIPTION "Header-only C++23 polymorphic containers with inline storage"
    LANGUAGES CXX
)

# Determine if this is a top-level project or included via add_subdirectory
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(INLINE_POLY_CONTAINERS_IS_TOP_LEVEL ON)
else()
    set(INLINE_POLY_CONTAINERS_IS_TOP_LEVEL OFF)
endif()

# Options (only relevant when building as top-level)
option(INLINE_POLY_CONTAINERS_BUILD_TESTS "Build unit tests" ${INLINE_POLY_CONTAINERS_IS_TOP_LEVEL})
option(INLINE_POLY_CONTAINERS_BUILD_EXAMPLES "Build example programs" ${INLINE_POLY_CONTAINERS_IS_TOP_LEVEL})

# Create header-only interface library
add_library(inline_poly_containers INTERFACE)
add_library(inline_poly_containers::inline_poly_containers ALIAS inline_poly_containers)

# Set include directories
target_include_directories(inline_poly_containers
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Require C++23
target_compile_features(inline_poly_containers INTERFACE cxx_std_23)

# Optionally build tests
if(INLINE_POLY_CONTAINERS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Optionally build examples
if(INLINE_POLY_CONTAINERS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
