// Copyright 2025 Dr. Matthias Hölzl
#include <iostream>
#include "../include/inline_poly.h"

// Base class with virtual destructor (required by PolymorphicBase concept)
struct Shape
{
    virtual ~Shape()                               = default;
    [[nodiscard]] virtual double area() const      = 0;
    [[nodiscard]] virtual const char* name() const = 0;
};

struct Circle : Shape
{
    double radius;
    explicit Circle(double r) : radius(r) {}

    [[nodiscard]] double area() const override
    {
        return 3.14159 * radius * radius;
    }
    [[nodiscard]] const char* name() const override
    {
        return "Circle";
    }
};

struct Rectangle : Shape
{
    double width, height;
    Rectangle(double w, double h) : width(w), height(h) {}

    [[nodiscard]] double area() const override
    {
        return width * height;
    }
    [[nodiscard]] const char* name() const override
    {
        return "Rectangle";
    }
};

struct Triangle : Shape
{
    double base, height;
    Triangle(double b, double h) : base(b), height(h) {}

    [[nodiscard]] double area() const override
    {
        return 0.5 * base * height;
    }
    [[nodiscard]] const char* name() const override
    {
        return "Triangle";
    }
};

int main()
{
    // Use type_list to compute slot size and alignment for all shape types
    using ShapeTypes = inline_poly::type_list<Circle, Rectangle, Triangle>;
    using Config     = inline_poly::slot_config<ShapeTypes>;

    // Create an array with 4 slots using the computed configuration
    inline_poly::poly_array<Shape, 4, Config::size, Config::alignment> shapes;

    // Emplace different shapes at different indices
    shapes.emplace<Circle>(0, 5.0);
    shapes.emplace<Rectangle>(1, 3.0, 4.0);
    shapes.emplace<Triangle>(2, 6.0, 2.0);
    // Index 3 remains nullptr

    // Iterate and display
    std::cout << "Shapes in the array:\n";
    std::cout << "  Container is "
              << (shapes.is_copyable() ? "copyable" : "not copyable") << "\n";
    std::cout << "  Container is "
              << (shapes.is_movable() ? "movable" : "not movable") << "\n\n";

    for (size_t i = 0; i < shapes.size(); ++i)
    {
        if (shapes[i] != nullptr)
        {
            std::cout << "  [" << i << "] " << shapes[i]->name()
                      << " - area: " << shapes[i]->area() << "\n";
        }
        else
        {
            std::cout << "  [" << i << "] (empty)\n";
        }
    }

    // The unified containers support copying when all contained types are
    // copyable
    std::cout << "\nCreating a copy of the array...\n";
    auto shapes_copy = shapes; // Safe copy - no UB with non-trivial types

    // Calculate total area
    std::cout << "\nTotal area: ";
    double total = 0.0;
    for (size_t i = 0; i < shapes.size(); ++i)
    {
        if (shapes[i] != nullptr) total += shapes[i]->area();
    }
    std::cout << total << "\n";

    // Replace an element
    std::cout << "\nReplacing Circle with a larger Rectangle...\n";
    shapes.emplace<Rectangle>(0, 10.0, 10.0);
    std::cout << "New shape at [0]: " << shapes[0]->name()
              << " - area: " << shapes[0]->area() << "\n";

    return 0;
}
