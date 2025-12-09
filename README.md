# inline-poly-containers

Header-only C++23 polymorphic containers with inline storage. Zero heap allocations for container storage, full polymorphism support, and automatic copy/move capability detection.

## Features

- **Two container types** - `array` (fixed-size, sparse) and `vector` (dynamic size with fixed capacity)
- **Zero heap allocations** - All objects stored inline using placement new
- **Full polymorphism** - Virtual dispatch works as expected
- **Type-safe copy/move** - Proper handling of non-trivially copyable types (std::string, std::vector, etc.)
- **Automatic capability detection** - Containers track whether their contents can be copied or moved
- **STL-compatible interface** - Familiar methods like `push_back()`, `emplace()`, iterators
- **C++23 concepts** - Compile-time type safety with `PolymorphicBase` and `FitsInSlot`
- **Single header** - Just include `inline_poly.h`, no dependencies

## Quick Start

```cpp
#include <inline_poly.h>

// Your polymorphic hierarchy
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

struct Circle : Shape {
    double radius;
    Circle(double r) : radius(r) {}
    double area() const override { return 3.14159 * radius * radius; }
};

struct Rectangle : Shape {
    double width, height;
    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const override { return width * height; }
};

int main() {
    // Determine slot size at compile time
    constexpr size_t SlotSize = std::max(sizeof(Circle), sizeof(Rectangle));

    // Fixed-size array - all slots pre-allocated
    inline_poly::array<Shape, 4, SlotSize> shapes_array;
    shapes_array.emplace<Circle>(0, 5.0);
    shapes_array.emplace<Rectangle>(1, 4.0, 6.0);

    // Dynamic vector - grows/shrinks up to capacity
    inline_poly::vector<Shape, 10, SlotSize> shapes_vec;
    shapes_vec.emplace_back<Circle>(5.0);
    shapes_vec.push_back(Rectangle(4.0, 6.0));

    // Both support polymorphic iteration
    for (Shape* shape : shapes_vec) {
        if (shape) std::cout << shape->area() << '\n';
    }

    // Containers can be copied if all contained types are copyable
    if (shapes_vec.is_copyable()) {
        auto copy = shapes_vec;
    }
}
```

## Container Types

### `inline_poly::array<Base, N, SlotSize, Alignment>`

Fixed-size array similar to `std::array`, but for polymorphic types:
- All N slots are pre-allocated
- Direct index access with `operator[]`
- No size tracking (conceptually always "full")
- Objects can be `nullptr`

```cpp
inline_poly::array<Animal, 10, sizeof(LargeDog)> zoo;
zoo.emplace<Dog>(0, "Buddy");
zoo.emplace<Cat>(5, "Whiskers");
// Slots 1-4, 6-9 are nullptr
```

### `inline_poly::vector<Base, Capacity, SlotSize, Alignment>`

Variable-size vector similar to `std::vector`, but with fixed capacity:
- Dynamic size from 0 to Capacity
- `push_back()`, `pop_back()`, `insert()`, `erase()`
- Size tracking with `size()` and `empty()`
- No reallocation (capacity is fixed)

```cpp
inline_poly::vector<Animal, 100, sizeof(LargeDog)> kennel;
kennel.emplace_back<Dog>("Rex");
kennel.emplace_back<Cat>("Mittens");
kennel.pop_back();  // Removes the cat
std::cout << kennel.size();  // Prints: 1
```

## Type-Safe Copy and Move

The containers use a type-erased operations system to safely copy and move objects, even when they contain non-trivially copyable members like `std::string` or `std::vector`:

```cpp
struct Widget {
    virtual ~Widget() = default;
    virtual void display() const = 0;
};

struct Label : Widget {
    std::string text;  // Non-trivially copyable
    Label(std::string t) : text(std::move(t)) {}
    void display() const override { std::cout << text; }
};

struct Canvas : Widget {
    std::unique_ptr<int[]> buffer;  // Move-only
    Canvas(size_t size) : buffer(std::make_unique<int[]>(size)) {}
    void display() const override { /* ... */ }
};

inline_poly::vector<Widget, 10, 256> widgets;
widgets.emplace_back<Label>("Hello");  // Copyable
widgets.emplace_back<Canvas>(1024);    // Move-only

// Container adapts to contents
std::cout << widgets.is_copyable();  // false (contains Canvas)
std::cout << widgets.is_movable();   // true

// Copying fails at runtime with an exception
try {
    auto copy = widgets;  // throws std::logic_error
} catch (const std::logic_error& e) {
    std::cout << "Cannot copy: contains move-only types\n";
}

// Moving always works
auto moved = std::move(widgets);  // OK
```

## Slot Size Utilities

The library provides utilities to compute the required slot size and alignment for a set of derived types:

```cpp
// Define your type hierarchy
using ShapeTypes = inline_poly::type_list<Circle, Rectangle, Triangle>;

// Get the maximum size and alignment
constexpr auto SlotSize = inline_poly::max_size_v<ShapeTypes>;
constexpr auto SlotAlign = inline_poly::max_alignment_v<ShapeTypes>;

// Or use the convenience struct
using Config = inline_poly::slot_config<ShapeTypes>;
inline_poly::array<Shape, 10, Config::size, Config::alignment> shapes;
```

## Requirements

- C++23 compiler (MSVC 19.30+, GCC 12+, Clang 16+)
- CMake 3.14+ (for building tests/examples)

## Installation

### Option 1: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    inline_poly_containers
    GIT_REPOSITORY https://github.com/yourusername/inline-poly-containers.git
    GIT_TAG main
)
FetchContent_MakeAvailable(inline_poly_containers)

target_link_libraries(your_target PRIVATE inline_poly_containers::inline_poly_containers)
```

### Option 2: Copy the header

Copy `include/inline_poly.h` to your project.

## API Reference

### Template Parameters

Both containers share similar template parameters:

| Parameter   | Description                                           |
|-------------|-------------------------------------------------------|
| `Base`      | Base class type (must have virtual destructor)       |
| `N`/`Capacity` | Number of slots (array) or max capacity (vector)  |
| `SlotSize`  | Size in bytes of each slot (default: `sizeof(Base)`) |
| `Alignment` | Alignment requirement (default: `alignof(Base)`)     |

### Common Methods

| Method                          | array         | vector            | Description               |
|---------------------------------|---------------|-------------------|---------------------------|
| `emplace<T>(index, args...)`    | Y             | -                 | Construct object at index |
| `emplace<T>(iterator, args...)` | -             | Y                 | Insert at position        |
| `emplace_back<T>(args...)`      | -             | Y                 | Append object             |
| `push_back(obj)`                | -             | Y                 | Copy/move append          |
| `pop_back()`                    | -             | Y                 | Remove last element       |
| `erase(pos)`                    | -             | Y                 | Remove at position        |
| `clear()`                       | Y             | Y                 | Destroy all objects       |
| `operator[]`                    | Y             | Y                 | Unchecked access          |
| `at()`                          | Y             | Y                 | Checked access            |
| `size()`                        | Y (returns N) | Y (returns count) | Number of elements        |
| `empty()`                       | Y (N==0)      | Y (size==0)       | Check if empty            |
| `capacity()`                    | -             | Y                 | Maximum capacity          |
| `is_copyable()`                 | Y             | Y                 | Can container be copied?  |
| `is_movable()`                  | Y             | Y                 | Can container be moved?   |

### Iterators

Both containers provide full iterator support:
- `begin()`, `end()`
- `cbegin()`, `cend()`
- `rbegin()`, `rend()`
- Range-based for loop support

## Examples

The `examples/` directory contains demonstrations organized by topic:

### Quick Start
- `quickstart.cpp` - Basic usage matching the README example

### Features
- `features/copy_move_semantics.cpp` - Automatic copy/move capability detection
- `features/simd_alignment.cpp` - Custom alignment for SIMD types
- `features/vector_operations.cpp` - Vector insert, erase, and iteration

### Use Cases
- `use_cases/entity_component_system.cpp` - ECS pattern implementation
- `use_cases/array_vs_vector.cpp` - Comparing array and vector usage

## Building Tests and Examples

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run tests
ctest --test-dir build -C Release

# Run examples
./build/examples/Release/quickstart
./build/examples/Release/copy_move_semantics
./build/examples/Release/entity_component_system
```

## Use Cases

Perfect for:
- **Embedded systems** - Predictable memory usage, no allocations
- **Real-time systems** - No allocation latency
- **Game development** - Entity pools, particle systems, component storage
- **High-performance computing** - Cache-friendly polymorphic collections

## Project Structure

```
inline-poly-containers/
├── include/
│   └── inline_poly.h              # Single header (containers + type operations)
├── tests/
│   ├── test_polymorphic_array.cpp
│   └── test_polymorphic_vector.cpp
├── examples/
│   ├── quickstart.cpp             # Basic usage
│   ├── features/
│   │   ├── copy_move_semantics.cpp
│   │   ├── simd_alignment.cpp
│   │   └── vector_operations.cpp
│   └── use_cases/
│       ├── entity_component_system.cpp
│       └── array_vs_vector.cpp
└── README.md
```

## Limitations

- **Compile-time configuration** - Capacity, slot size, and alignment must be specified when the container type is instantiated. Use `type_list` and `slot_config` to compute these values from your derived types.
- **No dynamic growth** - Containers cannot grow beyond their compile-time capacity. This is by design for predictable memory usage.
- **Runtime copy check** - Whether a container can be copied depends on the types it currently contains. Attempting to copy a container with move-only types throws `std::logic_error`.
- **Erase requires movability** - `erase()` throws if elements are non-movable since it must shift elements. Use `pop_back()` to remove elements from the end without requiring moves.

## License

MIT License - see [LICENSE](LICENSE) for details.
