// Copyright 2025 Dr. Matthias Hölzl
// no_heap_demo.cpp - Demonstrates zero heap allocations in inline_poly containers
//
// This example shows that poly_array and poly_vector perform ZERO heap
// allocations for all container operations. The allocation counter stays at zero
// throughout.

#include "../../include/inline_poly.h"

#include <atomic>
#include <iostream>
#include <new>

// =============================================================================
// Allocation Tracking Infrastructure
// =============================================================================

namespace
{
    std::atomic<std::size_t> g_allocation_count{0};
    std::atomic<std::size_t> g_total_bytes{0};
    bool                     g_tracking_enabled = false;

    void report_allocations(const char* phase)
    {
        // Disable tracking while printing to avoid counting iostream allocations
        bool was_enabled   = g_tracking_enabled;
        g_tracking_enabled = false;

        std::cout << "  [" << phase << "] Allocations: " << g_allocation_count
                  << ", Bytes: " << g_total_bytes << "\n";

        g_tracking_enabled = was_enabled;
    }

    void reset_counters()
    {
        g_allocation_count = 0;
        g_total_bytes      = 0;
    }
} // namespace

// Override global operator new to track allocations
void* operator new(std::size_t size)
{
    if (g_tracking_enabled)
    {
        g_allocation_count.fetch_add(1);
        g_total_bytes.fetch_add(size);
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t size)
{
    if (g_tracking_enabled)
    {
        g_allocation_count.fetch_add(1);
        g_total_bytes.fetch_add(size);
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}
void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}
void operator delete(void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}
void operator delete[](void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

// =============================================================================
// Test Types - Deliberately avoiding std::string to not confuse the demo
// =============================================================================

class Shape
{
public:
    virtual ~Shape()                 = default;
    virtual double area() const      = 0;
    virtual const char* name() const = 0;

protected:
    explicit Shape(int id) : id_(id) {}
    int id_;
};

class Circle : public Shape
{
public:
    Circle(int id, double radius) : Shape(id), radius_(radius) {}
    double area() const override
    {
        return 3.14159 * radius_ * radius_;
    }
    const char* name() const override
    {
        return "Circle";
    }

private:
    double radius_;
};

class Rectangle : public Shape
{
public:
    Rectangle(int id, double w, double h) : Shape(id), width_(w), height_(h) {}
    double area() const override
    {
        return width_ * height_;
    }
    const char* name() const override
    {
        return "Rectangle";
    }

private:
    double width_;
    double height_;
};

class Triangle : public Shape
{
public:
    Triangle(int id, double b, double h) : Shape(id), base_(b), height_(h) {}
    double area() const override
    {
        return 0.5 * base_ * height_;
    }
    const char* name() const override
    {
        return "Triangle";
    }

private:
    double base_;
    double height_;
};

// =============================================================================
// Demo Functions
// =============================================================================

void demo_poly_vector()
{
    std::cout << "\n=== poly_vector Operations ===\n\n";

    using ShapeVector = inline_poly::vector<Shape, 100, 64>;

    reset_counters();
    g_tracking_enabled = true;

    // Construction
    ShapeVector shapes;
    report_allocations("After construction");

    // Emplace elements
    for (int i = 0; i < 30; ++i)
    {
        if (i % 3 == 0)
            shapes.emplace_back<Circle>(i, static_cast<double>(i + 1));
        else if (i % 3 == 1)
            shapes.emplace_back<Rectangle>(i, static_cast<double>(i),
                                           static_cast<double>(i + 2));
        else
            shapes.emplace_back<Triangle>(i, static_cast<double>(i + 1),
                                          static_cast<double>(i + 3));
    }
    report_allocations("After 30 emplace_back");

    // Erase some elements
    shapes.erase(shapes.begin(), shapes.begin() + 10);
    report_allocations("After erasing 10 elements");

    // Pop back
    for (int i = 0; i < 5; ++i)
    {
        shapes.pop_back();
    }
    report_allocations("After 5 pop_back");

    // Move construction
    ShapeVector moved_shapes = std::move(shapes);
    report_allocations("After move construction");

    // Copy construction
    ShapeVector copied_shapes = moved_shapes;
    report_allocations("After copy construction");

    // Iterate and compute (poly_vector iterates over pointers)
    double total_area = 0.0;
    for (auto* shape : copied_shapes)
    {
        total_area += shape->area();
    }
    report_allocations("After iteration");

    // Clear
    copied_shapes.clear();
    moved_shapes.clear();
    report_allocations("After clear");

    g_tracking_enabled = false;

    std::cout << "\n  Total area computed: " << total_area << "\n";
    std::cout << "  Final allocation count: " << g_allocation_count << "\n";
}

void demo_poly_array()
{
    std::cout << "\n=== poly_array Operations ===\n\n";

    using ShapeArray = inline_poly::array<Shape, 50, 64>;

    reset_counters();
    g_tracking_enabled = true;

    // Construction
    ShapeArray shapes;
    report_allocations("After construction");

    // Emplace at specific indices
    for (std::size_t i = 0; i < 20; ++i)
    {
        if (i % 3 == 0)
            shapes.emplace<Circle>(i, static_cast<int>(i),
                                   static_cast<double>(i + 1));
        else if (i % 3 == 1)
            shapes.emplace<Rectangle>(i, static_cast<int>(i),
                                      static_cast<double>(i),
                                      static_cast<double>(i + 2));
        else
            shapes.emplace<Triangle>(i, static_cast<int>(i),
                                     static_cast<double>(i + 1),
                                     static_cast<double>(i + 3));
    }
    report_allocations("After 20 emplace");

    // Overwrite some elements (poly_array doesn't have erase, we overwrite)
    for (std::size_t i = 0; i < 5; ++i)
    {
        shapes.emplace<Circle>(i * 2, static_cast<int>(i + 100),
                               static_cast<double>(i + 100));
    }
    report_allocations("After overwriting 5 elements");

    // Move construction
    ShapeArray moved_shapes = std::move(shapes);
    report_allocations("After move construction");

    // Copy construction
    ShapeArray copied_shapes = moved_shapes;
    report_allocations("After copy construction");

    // Iterate and compute (poly_array iterates over pointers)
    double total_area = 0.0;
    for (auto* shape : copied_shapes)
    {
        if (shape)
        {
            total_area += shape->area();
        }
    }
    report_allocations("After iteration");

    // Clear
    copied_shapes.clear();
    moved_shapes.clear();
    report_allocations("After clear");

    g_tracking_enabled = false;

    std::cout << "\n  Total area computed: " << total_area << "\n";
    std::cout << "  Final allocation count: " << g_allocation_count << "\n";
}

int main()
{
    std::cout << "============================================================\n";
    std::cout << "     Zero Heap Allocation Demonstration\n";
    std::cout << "============================================================\n";
    std::cout << "\nThis demo tracks ALL heap allocations via operator new.\n";
    std::cout << "inline_poly containers use inline storage, so the allocation\n";
    std::cout << "counter should remain at ZERO throughout all operations.\n";

    demo_poly_vector();
    demo_poly_array();

    std::cout
        << "\n============================================================\n";
    std::cout << "     Summary\n";
    std::cout << "============================================================\n";
    std::cout << "\nKey Design Points:\n";
    std::cout << "  - All object storage is inline (stack or embedded)\n";
    std::cout
        << "  - Type operations use static instances (no registry allocation)\n";
    std::cout
        << "  - Placement new constructs objects in pre-allocated storage\n";
    std::cout
        << "  - Copy/move use type-erased operations with memcpy optimization\n";
    std::cout << "\nUse Cases:\n";
    std::cout << "  - Real-time systems (no allocation latency)\n";
    std::cout << "  - Embedded systems (predictable memory usage)\n";
    std::cout << "  - Game engines (cache-friendly, no GC pressure)\n";
    std::cout << "  - Audio/video processing (deterministic timing)\n";

    return 0;
}
