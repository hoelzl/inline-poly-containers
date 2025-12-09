// Copyright 2025 Dr. Matthias Hölzl
// Test that inline_poly containers perform zero heap allocations
// This test overrides global operator new to track all allocations

#include "../include/inline_poly.h"

#include <atomic>
#include <cstdlib>
#include <new>

// Global allocation tracking
namespace
{
    std::atomic<std::size_t> g_allocation_count{0};
    std::atomic<std::size_t> g_total_bytes{0};
    std::atomic<bool>        g_tracking_enabled{false};

    // RAII guard for tracking allocations in a scope
    class AllocationGuard
    {
    public:
        explicit AllocationGuard(const char* context) : context_(context)
        {
            start_count_ = g_allocation_count.load();
            start_bytes_ = g_total_bytes.load();
            g_tracking_enabled.store(true);
        }

        ~AllocationGuard()
        {
            g_tracking_enabled.store(false);
        }

        AllocationGuard(const AllocationGuard&)            = delete;
        AllocationGuard& operator=(const AllocationGuard&) = delete;

        [[nodiscard]] std::size_t allocations() const
        {
            return g_allocation_count.load() - start_count_;
        }

        [[nodiscard]] std::size_t bytes() const
        {
            return g_total_bytes.load() - start_bytes_;
        }

        [[nodiscard]] const char* context() const
        {
            return context_;
        }

    private:
        const char* context_;
        std::size_t start_count_;
        std::size_t start_bytes_;
    };
} // namespace

// Override global operator new to track allocations
void* operator new(std::size_t size)
{
    if (g_tracking_enabled.load())
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
    if (g_tracking_enabled.load())
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

// Now include doctest AFTER our operator new overrides
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// Test hierarchy - no std::string to avoid unrelated allocations
class Shape
{
public:
    virtual ~Shape()            = default;
    virtual double area() const = 0;
    virtual int id() const
    {
        return id_;
    }

protected:
    explicit Shape(int id) : id_(id) {}

private:
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
    double radius() const
    {
        return radius_;
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

private:
    double width_;
    double height_;
};

class Triangle : public Shape
{
public:
    Triangle(int id, double base, double height) :
        Shape(id), base_(base), height_(height)
    {}
    double area() const override
    {
        return 0.5 * base_ * height_;
    }

private:
    double base_;
    double height_;
};

// Configuration
constexpr std::size_t TestCapacity = 20;
constexpr std::size_t TestSlotSize = 64;

using TestVector = inline_poly::vector<Shape, TestCapacity, TestSlotSize>;
using TestArray  = inline_poly::array<Shape, TestCapacity, TestSlotSize>;

// =============================================================================
// poly_vector Tests
// =============================================================================

TEST_CASE("poly_vector - Zero allocations for default construction")
{
    AllocationGuard guard("vector default construction");

    TestVector vec;

    CHECK(guard.allocations() == 0);
    CHECK(vec.empty());
}

TEST_CASE("poly_vector - Zero allocations for emplace_back")
{
    AllocationGuard guard("vector emplace_back");

    TestVector vec;
    vec.emplace_back<Circle>(1, 5.0);
    vec.emplace_back<Rectangle>(2, 3.0, 4.0);
    vec.emplace_back<Triangle>(3, 6.0, 8.0);

    CHECK(guard.allocations() == 0);
    CHECK(vec.size() == 3);
}

TEST_CASE("poly_vector - Zero allocations for filling to capacity")
{
    AllocationGuard guard("vector fill to capacity");

    TestVector vec;
    for (std::size_t i = 0; i < TestCapacity; ++i)
    {
        vec.emplace_back<Circle>(static_cast<int>(i), static_cast<double>(i));
    }

    CHECK(guard.allocations() == 0);
    CHECK(vec.size() == TestCapacity);
}

TEST_CASE("poly_vector - Zero allocations for pop_back")
{
    TestVector vec;
    vec.emplace_back<Circle>(1, 5.0);
    vec.emplace_back<Rectangle>(2, 3.0, 4.0);

    AllocationGuard guard("vector pop_back");
    vec.pop_back();

    CHECK(guard.allocations() == 0);
    CHECK(vec.size() == 1);
}

TEST_CASE("poly_vector - Zero allocations for erase")
{
    TestVector vec;
    vec.emplace_back<Circle>(1, 5.0);
    vec.emplace_back<Rectangle>(2, 3.0, 4.0);
    vec.emplace_back<Triangle>(3, 6.0, 8.0);

    AllocationGuard guard("vector erase");
    vec.erase(vec.begin() + 1);

    CHECK(guard.allocations() == 0);
    CHECK(vec.size() == 2);
}

TEST_CASE("poly_vector - Zero allocations for clear")
{
    TestVector vec;
    vec.emplace_back<Circle>(1, 5.0);
    vec.emplace_back<Rectangle>(2, 3.0, 4.0);

    AllocationGuard guard("vector clear");
    vec.clear();

    CHECK(guard.allocations() == 0);
    CHECK(vec.empty());
}

TEST_CASE("poly_vector - Zero allocations for move construction")
{
    TestVector vec1;
    vec1.emplace_back<Circle>(1, 5.0);
    vec1.emplace_back<Rectangle>(2, 3.0, 4.0);

    AllocationGuard guard("vector move construction");
    TestVector      vec2 = std::move(vec1);

    CHECK(guard.allocations() == 0);
    CHECK(vec2.size() == 2);
}

TEST_CASE("poly_vector - Zero allocations for move assignment")
{
    TestVector vec1;
    vec1.emplace_back<Circle>(1, 5.0);
    vec1.emplace_back<Rectangle>(2, 3.0, 4.0);

    TestVector vec2;
    vec2.emplace_back<Triangle>(3, 6.0, 8.0);

    AllocationGuard guard("vector move assignment");
    vec2 = std::move(vec1);

    CHECK(guard.allocations() == 0);
    CHECK(vec2.size() == 2);
}

TEST_CASE("poly_vector - Zero allocations for copy construction")
{
    TestVector vec1;
    vec1.emplace_back<Circle>(1, 5.0);
    vec1.emplace_back<Rectangle>(2, 3.0, 4.0);

    AllocationGuard guard("vector copy construction");
    TestVector      vec2 = vec1;

    CHECK(guard.allocations() == 0);
    CHECK(vec2.size() == 2);
}

TEST_CASE("poly_vector - Zero allocations for copy assignment")
{
    TestVector vec1;
    vec1.emplace_back<Circle>(1, 5.0);
    vec1.emplace_back<Rectangle>(2, 3.0, 4.0);

    TestVector vec2;
    vec2.emplace_back<Triangle>(3, 6.0, 8.0);

    AllocationGuard guard("vector copy assignment");
    vec2 = vec1;

    CHECK(guard.allocations() == 0);
    CHECK(vec2.size() == 2);
}

TEST_CASE("poly_vector - Zero allocations for iteration")
{
    TestVector vec;
    vec.emplace_back<Circle>(1, 5.0);
    vec.emplace_back<Rectangle>(2, 3.0, 4.0);
    vec.emplace_back<Triangle>(3, 6.0, 8.0);

    AllocationGuard guard("vector iteration");
    double          total_area = 0.0;
    for (auto* shape : vec)
    {
        total_area += shape->area();
    }

    CHECK(guard.allocations() == 0);
    CHECK(total_area > 0.0);
}

TEST_CASE("poly_vector - Zero allocations for element access")
{
    TestVector vec;
    vec.emplace_back<Circle>(1, 5.0);
    vec.emplace_back<Rectangle>(2, 3.0, 4.0);

    AllocationGuard        guard("vector element access");
    [[maybe_unused]] auto& front = vec.front();
    [[maybe_unused]] auto& back  = vec.back();
    [[maybe_unused]] auto& elem  = vec[0];
    [[maybe_unused]] auto& at    = vec.at(1);

    CHECK(guard.allocations() == 0);
}

// =============================================================================
// poly_array Tests
// =============================================================================

TEST_CASE("poly_array - Zero allocations for default construction")
{
    AllocationGuard guard("array default construction");

    TestArray arr;

    CHECK(guard.allocations() == 0);
}

TEST_CASE("poly_array - Zero allocations for emplace")
{
    AllocationGuard guard("array emplace");

    TestArray arr;
    arr.emplace<Circle>(0, 1, 5.0);
    arr.emplace<Rectangle>(1, 2, 3.0, 4.0);
    arr.emplace<Triangle>(2, 3, 6.0, 8.0);

    CHECK(guard.allocations() == 0);
    CHECK(arr[0] != nullptr);
    CHECK(arr[1] != nullptr);
    CHECK(arr[2] != nullptr);
}

TEST_CASE("poly_array - Zero allocations for filling all slots")
{
    AllocationGuard guard("array fill all slots");

    TestArray arr;
    for (std::size_t i = 0; i < TestCapacity; ++i)
    {
        arr.emplace<Circle>(i, static_cast<int>(i), static_cast<double>(i));
    }

    CHECK(guard.allocations() == 0);
    // All slots filled
    for (std::size_t i = 0; i < TestCapacity; ++i)
    {
        CHECK(arr[i] != nullptr);
    }
}

TEST_CASE("poly_array - Zero allocations for overwriting elements")
{
    TestArray arr;
    arr.emplace<Circle>(0, 1, 5.0);
    arr.emplace<Rectangle>(1, 2, 3.0, 4.0);

    AllocationGuard guard("array overwrite");
    // Overwriting with a new element (destroys old, constructs new)
    arr.emplace<Triangle>(0, 10, 6.0, 8.0);

    CHECK(guard.allocations() == 0);
    CHECK(arr[0] != nullptr);
}

TEST_CASE("poly_array - Zero allocations for clear")
{
    TestArray arr;
    arr.emplace<Circle>(0, 1, 5.0);
    arr.emplace<Rectangle>(1, 2, 3.0, 4.0);

    AllocationGuard guard("array clear");
    arr.clear();

    CHECK(guard.allocations() == 0);
    CHECK(arr[0] == nullptr);
    CHECK(arr[1] == nullptr);
}

TEST_CASE("poly_array - Zero allocations for move construction")
{
    TestArray arr1;
    arr1.emplace<Circle>(0, 1, 5.0);
    arr1.emplace<Rectangle>(1, 2, 3.0, 4.0);

    AllocationGuard guard("array move construction");
    TestArray       arr2 = std::move(arr1);

    CHECK(guard.allocations() == 0);
    CHECK(arr2[0] != nullptr);
    CHECK(arr2[1] != nullptr);
}

TEST_CASE("poly_array - Zero allocations for move assignment")
{
    TestArray arr1;
    arr1.emplace<Circle>(0, 1, 5.0);
    arr1.emplace<Rectangle>(1, 2, 3.0, 4.0);

    TestArray arr2;
    arr2.emplace<Triangle>(0, 3, 6.0, 8.0);

    AllocationGuard guard("array move assignment");
    arr2 = std::move(arr1);

    CHECK(guard.allocations() == 0);
    CHECK(arr2[0] != nullptr);
    CHECK(arr2[1] != nullptr);
}

TEST_CASE("poly_array - Zero allocations for copy construction")
{
    TestArray arr1;
    arr1.emplace<Circle>(0, 1, 5.0);
    arr1.emplace<Rectangle>(1, 2, 3.0, 4.0);

    AllocationGuard guard("array copy construction");
    TestArray       arr2 = arr1;

    CHECK(guard.allocations() == 0);
    CHECK(arr2[0] != nullptr);
    CHECK(arr2[1] != nullptr);
}

TEST_CASE("poly_array - Zero allocations for copy assignment")
{
    TestArray arr1;
    arr1.emplace<Circle>(0, 1, 5.0);
    arr1.emplace<Rectangle>(1, 2, 3.0, 4.0);

    TestArray arr2;
    arr2.emplace<Triangle>(0, 3, 6.0, 8.0);

    AllocationGuard guard("array copy assignment");
    arr2 = arr1;

    CHECK(guard.allocations() == 0);
    CHECK(arr2[0] != nullptr);
    CHECK(arr2[1] != nullptr);
}

TEST_CASE("poly_array - Zero allocations for iteration")
{
    TestArray arr;
    arr.emplace<Circle>(0, 1, 5.0);
    arr.emplace<Rectangle>(1, 2, 3.0, 4.0);
    arr.emplace<Triangle>(2, 3, 6.0, 8.0);

    AllocationGuard guard("array iteration");
    double          total_area = 0.0;
    for (auto* shape : arr)
    {
        if (shape)
        {
            total_area += shape->area();
        }
    }

    CHECK(guard.allocations() == 0);
    CHECK(total_area > 0.0);
}

TEST_CASE("poly_array - Zero allocations for element access")
{
    TestArray arr;
    arr.emplace<Circle>(0, 1, 5.0);
    arr.emplace<Rectangle>(1, 2, 3.0, 4.0);

    AllocationGuard        guard("array element access");
    [[maybe_unused]] auto* elem0 = arr[0];
    [[maybe_unused]] auto* at1   = arr.at(1);

    CHECK(guard.allocations() == 0);
}
