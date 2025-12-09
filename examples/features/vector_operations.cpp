// Copyright 2025 Dr. Matthias Hölzl
#include <iostream>
#include <memory>
#include <string>
#include "../../include/inline_poly.h"

// Base class for shapes
class Shape
{
public:
    virtual ~Shape()                 = default;
    virtual double area() const      = 0;
    virtual std::string name() const = 0;
    virtual void draw() const        = 0;
};

// Circle implementation
class Circle : public Shape
{
public:
    explicit Circle(double radius) : radius_(radius) {}

    double area() const override
    {
        return 3.14159 * radius_ * radius_;
    }
    std::string name() const override
    {
        return "Circle";
    }
    void draw() const override
    {
        std::cout << "Drawing Circle with radius " << radius_ << std::endl;
    }

private:
    double radius_;
};

// Rectangle implementation
class Rectangle : public Shape
{
public:
    Rectangle(double width, double height) : width_(width), height_(height) {}

    double area() const override
    {
        return width_ * height_;
    }
    std::string name() const override
    {
        return "Rectangle";
    }
    void draw() const override
    {
        std::cout << "Drawing Rectangle " << width_ << "x" << height_
                  << std::endl;
    }

private:
    double width_, height_;
};

// Triangle implementation
class Triangle : public Shape
{
public:
    Triangle(double base, double height) : base_(base), height_(height) {}

    double area() const override
    {
        return 0.5 * base_ * height_;
    }
    std::string name() const override
    {
        return "Triangle";
    }
    void draw() const override
    {
        std::cout << "Drawing Triangle with base " << base_ << " and height "
                  << height_ << std::endl;
    }

private:
    double base_, height_;
};

int main()
{
    std::cout << "=== PolymorphicVector Demo ===" << std::endl;
    std::cout << "A fixed-capacity vector for polymorphic types with no heap "
                 "allocations\n"
              << std::endl;

    using AllShapes = inline_poly::type_list<Shape, Circle, Rectangle, Triangle>;
    constexpr size_t SlotSize = inline_poly::max_size_v<AllShapes>;

    // Create a PolymorphicVector with capacity for 10 shapes
    // SlotSize must be large enough for the largest derived class
    constexpr size_t MaxShapes = 10;
    using ShapeVector = inline_poly::poly_vector<Shape, MaxShapes, SlotSize>;

    ShapeVector shapes;

    std::cout << "Initial state:" << std::endl;
    std::cout << "  Size: " << shapes.size() << std::endl;
    std::cout << "  Capacity: " << shapes.capacity() << std::endl;
    std::cout << "  Empty: " << (shapes.empty() ? "yes" : "no") << std::endl;

    // Add shapes dynamically
    std::cout << "\nAdding shapes dynamically..." << std::endl;
    shapes.emplace_back<Circle>(5.0);
    shapes.emplace_back<Rectangle>(4.0, 6.0);
    shapes.emplace_back<Triangle>(3.0, 4.0);

    std::cout << "After adding 3 shapes:" << std::endl;
    std::cout << "  Size: " << shapes.size() << std::endl;
    std::cout << "  Empty: " << (shapes.empty() ? "yes" : "no") << std::endl;

    // Insert a shape at the beginning
    std::cout << "\nInserting a circle at the beginning..." << std::endl;
    shapes.emplace<Circle>(shapes.begin(), 2.5);

    // Iterate and draw all shapes
    std::cout << "\nIterating through all shapes:" << std::endl;
    for (size_t i = 0; i < shapes.size(); ++i)
    {
        Shape* shape = shapes[i];
        std::cout << "  [" << i << "] " << shape->name()
                  << " - Area: " << shape->area() << std::endl;
        std::cout << "      ";
        shape->draw();
    }

    // Use range-based for loop
    std::cout << "\nUsing range-based for loop:" << std::endl;
    double total_area = 0.0;
    for (Shape* shape : shapes)
    {
        total_area += shape->area();
    }
    std::cout << "Total area of all shapes: " << total_area << std::endl;

    // Demonstrate erase
    std::cout << "\nErasing the second shape..." << std::endl;
    shapes.erase(shapes.begin() + 1);

    std::cout << "Remaining shapes:" << std::endl;
    for (Shape* shape : shapes)
    {
        std::cout << "  - " << shape->name() << " (area: " << shape->area() << ")"
                  << std::endl;
    }

    // Demonstrate pop_back
    std::cout << "\nRemoving last shape..." << std::endl;
    shapes.pop_back();
    std::cout << "Size after pop_back: " << shapes.size() << std::endl;

    // Add more shapes to show dynamic growth
    std::cout << "\nAdding more shapes..." << std::endl;
    shapes.push_back(Rectangle(10.0, 5.0));
    shapes.push_back(Circle(7.0));
    shapes.emplace_back<Triangle>(8.0, 6.0);

    std::cout << "Final size: " << shapes.size() << std::endl;

    // Clear all shapes
    std::cout << "\nClearing all shapes..." << std::endl;
    shapes.clear();
    std::cout << "Size after clear: " << shapes.size() << std::endl;
    std::cout << "Capacity remains: " << shapes.capacity() << std::endl;

    // Demonstrate resize with nullptr elements
    std::cout << "\nResizing to 5 (creates nullptr elements)..." << std::endl;
    shapes.resize(5);
    // After resize, all 5 elements are nullptr
    // Now we'll add shapes at specific positions
    shapes.emplace_back<Circle>(3.0);         // This becomes element 5
    shapes.emplace_back<Rectangle>(2.0, 3.0); // This becomes element 6

    std::cout << "After resize and selective placement:" << std::endl;
    for (size_t i = 0; i < shapes.size(); ++i)
    {
        if (shapes[i] != nullptr)
        {
            std::cout << "  [" << i << "] " << shapes[i]->name() << std::endl;
        }
        else
        {
            std::cout << "  [" << i << "] nullptr" << std::endl;
        }
    }

    std::cout << "\n=== Key Benefits ===" << std::endl;
    std::cout << "1. No heap allocations - all objects stored inline"
              << std::endl;
    std::cout << "2. std::vector-like interface for familiarity" << std::endl;
    std::cout << "3. Dynamic size tracking (unlike PolymorphicArray)"
              << std::endl;
    std::cout << "4. Supports insert/erase operations" << std::endl;
    std::cout << "5. Perfect for real-time systems with bounded collections"
              << std::endl;

    return 0;
}
