# Fetch doctest
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)

# Suppress deprecation warning from doctest's old cmake_minimum_required
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(doctest)
set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)

# Create test executables
add_executable(polymorphic_array_tests
    test_polymorphic_array.cpp
)

add_executable(polymorphic_vector_tests
    test_polymorphic_vector.cpp
)

add_executable(no_allocation_tests
    test_no_allocations.cpp
)

target_link_libraries(polymorphic_array_tests
    PRIVATE
        inline_poly_containers::inline_poly_containers
        doctest::doctest
)

target_link_libraries(polymorphic_vector_tests
    PRIVATE
        inline_poly_containers::inline_poly_containers
        doctest::doctest
)

target_link_libraries(no_allocation_tests
    PRIVATE
        inline_poly_containers::inline_poly_containers
        doctest::doctest
)

# Register with CTest
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(polymorphic_array_tests)
doctest_discover_tests(polymorphic_vector_tests)
doctest_discover_tests(no_allocation_tests)
