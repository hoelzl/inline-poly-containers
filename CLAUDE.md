# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (from project root)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build everything
cmake --build build --config Release

# Run all tests
ctest --test-dir build -C Release

# Run a single test executable
./build/tests/Release/polymorphic_array_tests.exe
./build/tests/Release/polymorphic_vector_tests.exe

# Run specific test case (doctest supports filtering)
./build/tests/Release/polymorphic_array_tests.exe --test-case="*emplace*"

# Run examples
./build/examples/Release/quickstart.exe
```

## Architecture

This is a header-only C++23 library providing polymorphic containers with inline storage. The entire implementation is in `include/inline_poly.h`.

### Core Components

**Type Operations System** (`type_operations`, `type_operations_factory`):
- Type-erased operations (destroy, move, copy) for safe polymorphic manipulation
- Static factory pattern avoids heap allocations for operation structs
- Tracks type capabilities: `is_trivially_copyable`, `is_copy_constructible`, `is_move_constructible`

**Container Classes**:
- `poly_array<Base, N, SlotSize, Alignment>` - Fixed-size sparse array, slots can be nullptr
- `poly_vector<Base, Capacity, SlotSize, Alignment>` - Dynamic size with fixed capacity

**Key Design Patterns**:
- Slot-based storage: Each slot is a contiguous byte array sized to fit derived types
- Runtime capability tracking: Containers update `can_copy_`/`can_move_` as elements are added/removed
- Pointer cache for iterators: Avoids complex iterator implementations

### Concepts

- `PolymorphicBase<T>` - Requires virtual destructor
- `FitsInSlot<Derived, Base, SlotSize, Alignment>` - Compile-time check that derived type fits in slot

### Type Aliases

`inline_poly::array` and `inline_poly::vector` are the public-facing aliases for `poly_array` and `poly_vector`.

## Testing

Tests use doctest (fetched via CMake FetchContent). Test files:
- `tests/test_polymorphic_array.cpp` - Tests for `poly_array`
- `tests/test_polymorphic_vector.cpp` - Tests for `poly_vector`
- `tests/test_no_allocations.cpp` - Verifies zero heap allocation guarantee

## Key Constraints

- Slot size and alignment are compile-time fixed - use `type_list` and `slot_config` to compute from derived types
- No dynamic growth beyond compile-time capacity
- Copy operations throw `std::logic_error` if container holds non-copyable types
- `erase()` throws if elements are non-movable; use `pop_back()` to remove elements from the end
- After each major change, check whether the @README.md file needs to be updated
- After each major change, run the tests and all examples to verify they still work