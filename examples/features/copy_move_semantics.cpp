// Copyright 2025 Dr. Matthias Hölzl
// copy_move_semantics.cpp - Demonstrates automatic copy/move capability detection
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "../../include/inline_poly.h"

// ===== Base class =====
class Widget
{
public:
    virtual ~Widget()                = default;
    virtual void display() const     = 0;
    virtual std::string type() const = 0;
};

// ===== Copyable widget (contains std::string) =====
class Label : public Widget
{
public:
    explicit Label(const std::string& text) : text_(text) {}

    void display() const override
    {
        std::cout << "Label: \"" << text_ << "\"";
    }

    std::string type() const override
    {
        return "Label";
    }

private:
    std::string text_; // Non-trivially copyable
};

// ===== Another copyable widget (contains vector) =====
class ListBox : public Widget
{
public:
    explicit ListBox(std::vector<std::string> items) : items_(std::move(items)) {}

    void display() const override
    {
        std::cout << "ListBox with " << items_.size() << " items";
        if (!items_.empty())
        {
            std::cout << " [first: \"" << items_[0] << "\"]";
        }
    }

    std::string type() const override
    {
        return "ListBox";
    }

private:
    std::vector<std::string> items_; // Non-trivially copyable
};

// ===== Move-only widget (contains unique_ptr) =====
class Canvas : public Widget
{
public:
    Canvas(int width, int height) :
        width_(width), height_(height),
        buffer_(std::make_unique<std::vector<uint8_t>>(width * height))
    {}

    // Delete copy operations
    Canvas(const Canvas&)            = delete;
    Canvas& operator=(const Canvas&) = delete;

    // Enable move operations
    Canvas(Canvas&&)            = default;
    Canvas& operator=(Canvas&&) = default;

    void display() const override
    {
        std::cout << "Canvas " << width_ << "x" << height_;
        if (buffer_)
        {
            std::cout << " [buffer: " << buffer_->size() << " bytes]";
        }
    }

    std::string type() const override
    {
        return "Canvas";
    }

private:
    int                                   width_, height_;
    std::unique_ptr<std::vector<uint8_t>> buffer_; // Makes class move-only
};

// ===== Helper functions =====

template <typename Container>
void display_widgets(const std::string& title, const Container& widgets)
{
    std::cout << "\n" << title << ":\n";
    std::cout << "  Container capabilities:\n";
    std::cout << "    - Copyable: " << (widgets.is_copyable() ? "YES" : "NO")
              << "\n";
    std::cout << "    - Movable: " << (widgets.is_movable() ? "YES" : "NO")
              << "\n";
    std::cout << "  Contents:\n";

    size_t count = 0;
    for (size_t i = 0; i < widgets.size(); ++i)
    {
        if (widgets[i])
        {
            std::cout << "    [" << i << "] ";
            widgets[i]->display();
            std::cout << "\n";
            count++;
        }
    }
    if (count == 0)
    {
        std::cout << "    (empty)\n";
    }
}

void display_vector_widgets(
    const std::string&                               title,
    const inline_poly::poly_vector<Widget, 10, 256>& widgets)
{
    std::cout << "\n" << title << ":\n";
    std::cout << "  Size: " << widgets.size() << "/" << widgets.capacity()
              << "\n";
    std::cout << "  Container capabilities:\n";
    std::cout << "    - Copyable: " << (widgets.is_copyable() ? "YES" : "NO")
              << "\n";
    std::cout << "    - Movable: " << (widgets.is_movable() ? "YES" : "NO")
              << "\n";
    std::cout << "  Contents:\n";

    for (size_t i = 0; i < widgets.size(); ++i)
    {
        std::cout << "    [" << i << "] ";
        widgets[i]->display();
        std::cout << "\n";
    }
}

// ===== Demo 1: Automatic capability detection =====
void demo_capability_detection()
{
    std::cout << "===== DEMO 1: Automatic Capability Detection =====\n";
    std::cout << "Shows how containers adapt based on contained types\n";

    using WidgetTypes = inline_poly::type_list<Label, ListBox, Canvas>;
    constexpr size_t MaxWidgetSize = inline_poly::max_size_v<WidgetTypes>;

    // Start with an empty array
    inline_poly::poly_array<Widget, 5, MaxWidgetSize> widgets;
    display_widgets("Empty container", widgets);

    // Add copyable widgets only
    std::cout << "\n>>> Adding copyable widgets (Label, ListBox)...\n";
    widgets.emplace<Label>(0, "File Menu");
    widgets.emplace<ListBox>(1, std::vector<std::string>{"Open", "Save", "Exit"});
    widgets.emplace<Label>(2, "Status: Ready");

    display_widgets("With copyable widgets only", widgets);

    // Container is copyable, so we can copy it
    std::cout << "\n>>> Copying container...\n";
    auto widgets_copy = widgets;
    display_widgets("Copy of container", widgets_copy);

    // Now add a move-only widget
    std::cout << "\n>>> Adding move-only widget (Canvas) to original...\n";
    widgets.emplace<Canvas>(3, 800, 600);

    display_widgets("After adding Canvas (move-only)", widgets);

    // Now copying should fail
    std::cout << "\n>>> Attempting to copy container with move-only widget...\n";
    try
    {
        auto widgets_copy2 = widgets;
        std::cout << "ERROR: Copy should have failed!\n";
    }
    catch (const std::logic_error& e)
    {
        std::cout << "Copy failed as expected: " << e.what() << "\n";
    }

    // But moving still works
    std::cout << "\n>>> Moving container with move-only widget...\n";
    auto widgets_moved = std::move(widgets);
    display_widgets("Moved container", widgets_moved);
    display_widgets("Original after move", widgets);
}

// ===== Demo 2: Vector with dynamic type mixing =====
void demo_vector_operations()
{
    std::cout << "\n===== DEMO 2: Vector with Dynamic Type Mixing =====\n";
    std::cout
        << "Shows safe insertion/removal with non-trivially copyable types\n";

    constexpr size_t MaxWidgetSize = 256; // Large enough for all widgets

    inline_poly::poly_vector<Widget, 10, MaxWidgetSize> ui_elements;

    // Add various widgets
    std::cout << "\n>>> Building UI element list...\n";
    ui_elements.emplace_back<Label>("Title Bar");
    ui_elements.emplace_back<ListBox>(
        std::vector<std::string>{"Item 1", "Item 2", "Item 3"});
    ui_elements.emplace_back<Label>("Footer");

    display_vector_widgets("Initial UI elements (all copyable)", ui_elements);

    // Copy the vector
    std::cout << "\n>>> Copying vector...\n";
    auto ui_backup = ui_elements;
    display_vector_widgets("Backup copy", ui_backup);

    // Insert in the middle (tests safe shifting without memcpy)
    std::cout << "\n>>> Inserting new element at position 1...\n";
    ui_elements.emplace<Label>(ui_elements.begin() + 1, "Toolbar");

    display_vector_widgets("After insertion", ui_elements);
    display_vector_widgets("Backup unchanged", ui_backup);

    // Add a move-only widget
    std::cout << "\n>>> Adding Canvas (move-only)...\n";
    ui_elements.emplace_back<Canvas>(1920, 1080);

    display_vector_widgets("After adding Canvas", ui_elements);

    // Try to copy (should fail)
    std::cout << "\n>>> Attempting to copy vector with move-only element...\n";
    try
    {
        auto ui_copy = ui_elements;
        std::cout << "ERROR: Copy should have failed!\n";
    }
    catch (const std::logic_error& e)
    {
        std::cout << "Copy failed as expected: " << e.what() << "\n";
    }

    // But we can still move
    std::cout << "\n>>> Moving vector...\n";
    auto ui_moved = std::move(ui_elements);
    std::cout << "Move succeeded. Original size: " << ui_elements.size()
              << ", Moved size: " << ui_moved.size() << "\n";

    // Erase from the middle (tests safe shifting)
    std::cout << "\n>>> Erasing elements 1-2...\n";
    ui_moved.erase(ui_moved.begin() + 1, ui_moved.begin() + 3);
    display_vector_widgets("After erasure", ui_moved);
}

// ===== Global counters for performance demo =====
namespace
{
    int g_perf_constructions = 0;
    int g_perf_copies        = 0;
    int g_perf_moves         = 0;
    int g_perf_destructions  = 0;

    void reset_counters()
    {
        g_perf_constructions = g_perf_copies = g_perf_moves =
            g_perf_destructions              = 0;
    }

    void report_counters()
    {
        std::cout << "  Operations performed:\n";
        std::cout << "    Constructions: " << g_perf_constructions << "\n";
        std::cout << "    Copies: " << g_perf_copies << "\n";
        std::cout << "    Moves: " << g_perf_moves << "\n";
        std::cout << "    Destructions: " << g_perf_destructions << "\n";
    }
} // namespace

class InstrumentedWidget : public Widget
{
public:
    explicit InstrumentedWidget(std::string name) : name_(std::move(name))
    {
        g_perf_constructions++;
    }

    InstrumentedWidget(const InstrumentedWidget& other) : name_(other.name_)
    {
        g_perf_copies++;
    }

    InstrumentedWidget(InstrumentedWidget&& other) noexcept :
        name_(std::move(other.name_))
    {
        g_perf_moves++;
    }

    ~InstrumentedWidget() override
    {
        g_perf_destructions++;
    }

    void display() const override
    {
        std::cout << "Widget[" << name_ << "]";
    }

    std::string type() const override
    {
        return "Instrumented";
    }

private:
    std::string name_; // Non-trivially copyable
};

// ===== Demo 3: Performance characteristics =====
void demo_performance()
{
    std::cout << "\n===== DEMO 3: Performance Characteristics =====\n";
    std::cout << "Shows that operations are type-safe without heap allocations\n";

    reset_counters();

    {
        std::cout << "\n>>> Creating vector and adding 3 widgets...\n";
        inline_poly::poly_vector<Widget, 10, sizeof(InstrumentedWidget)> vec;
        vec.emplace_back<InstrumentedWidget>("First");
        vec.emplace_back<InstrumentedWidget>("Second");
        vec.emplace_back<InstrumentedWidget>("Third");
        report_counters();

        std::cout << "\n>>> Inserting at position 1 (causes shift)...\n";
        reset_counters();
        vec.emplace<InstrumentedWidget>(vec.begin() + 1, "Inserted");
        report_counters();
        std::cout << "  Note: Moves for shifting, no copies!\n";

        std::cout << "\n>>> Copying entire vector...\n";
        reset_counters();
        auto vec2 = vec;
        report_counters();

        std::cout << "\n>>> Moving entire vector...\n";
        reset_counters();
        auto vec3 = std::move(vec2);
        report_counters();

        std::cout << "\n>>> Vectors going out of scope...\n";
        reset_counters();
    }
    report_counters();
    std::cout << "  Note: All objects properly destroyed\n";
}

// ===== Main =====
int main()
{
    std::cout << "Unified Polymorphic Containers - Copy/Move Demo\n";
    std::cout << "===============================================\n";
    std::cout << "Demonstrates automatic adaptation to contained types' "
                 "capabilities\n\n";

    try
    {
        demo_capability_detection();
        demo_vector_operations();
        demo_performance();

        std::cout << "\n===== Summary =====\n";
        std::cout
            << "✓ Containers automatically detect if contents are copyable\n";
        std::cout << "✓ Type-safe operations without std::memcpy\n";
        std::cout << "✓ Proper handling of non-trivially copyable types (string, "
                     "vector)\n";
        std::cout << "✓ Support for move-only types (unique_ptr)\n";
        std::cout << "✓ Zero heap allocations for container storage\n";
        std::cout << "✓ Strong exception safety guarantees\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
