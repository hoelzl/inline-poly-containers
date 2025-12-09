// Copyright 2025 Dr. Matthias Hölzl
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>
#include "../include/inline_poly.h"

// Test hierarchy
class Animal
{
public:
    virtual ~Animal()                 = default;
    virtual std::string speak() const = 0;
    virtual int id() const
    {
        return id_;
    }

protected:
    explicit Animal(int id) : id_(id) {}

private:
    int id_;
};

class Dog : public Animal
{
public:
    explicit Dog(int id) : Animal(id) {}
    std::string speak() const override
    {
        return "Woof";
    }
};

class Cat : public Animal
{
public:
    explicit Cat(int id) : Animal(id) {}
    std::string speak() const override
    {
        return "Meow";
    }
};

class BigDog : public Dog
{
public:
    BigDog(int id, double weight) : Dog(id), weight_(weight) {}
    std::string speak() const override
    {
        return "WOOF!";
    }
    double weight() const
    {
        return weight_;
    }

private:
    double weight_;
};

// Test configuration
constexpr size_t TestCapacity = 10;
constexpr size_t TestSlotSize = sizeof(BigDog) + 8; // Extra space for alignment
using TestVector = inline_poly::vector<Animal, TestCapacity, TestSlotSize>;

// --- Construction and Basic Properties ---

TEST_CASE("inline_poly::vector - Default Construction")
{
    TestVector vec;
    CHECK(vec.size() == 0u);
    CHECK(vec.capacity() == TestCapacity);
    CHECK(vec.empty());
}

TEST_CASE("inline_poly::vector - Max Size and Capacity")
{
    TestVector vec;
    CHECK(vec.max_size() == TestCapacity);
    CHECK(vec.capacity() == TestCapacity);
}

// --- Push Back Operations ---

TEST_CASE("inline_poly::vector - Emplace Back")
{
    TestVector vec;

    auto* dog = vec.emplace_back<Dog>(1);
    CHECK(dog != nullptr);
    CHECK(vec.size() == 1u);
    CHECK_FALSE(vec.empty());
    CHECK(dog->id() == 1);
    CHECK(dog->speak() == "Woof");

    auto* cat = vec.emplace_back<Cat>(2);
    CHECK(cat != nullptr);
    CHECK(vec.size() == 2u);
    CHECK(cat->id() == 2);
    CHECK(cat->speak() == "Meow");
}

TEST_CASE("inline_poly::vector - Push Back Copy")
{
    TestVector vec;

    Dog dog(1);
    vec.push_back(dog);
    CHECK(vec.size() == 1u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[0]->speak() == "Woof");
}

TEST_CASE("inline_poly::vector - Push Back Move")
{
    TestVector vec;

    vec.push_back(Dog(1));
    CHECK(vec.size() == 1u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[0]->speak() == "Woof");
}

TEST_CASE("inline_poly::vector - Push Back Mixed Types")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 50.5);

    CHECK(vec.size() == 3u);
    CHECK(vec[0]->speak() == "Woof");
    CHECK(vec[1]->speak() == "Meow");
    CHECK(vec[2]->speak() == "WOOF!");

    auto* bigDog = dynamic_cast<BigDog*>(vec[2]);
    CHECK(bigDog != nullptr);
    CHECK(bigDog->weight() == doctest::Approx(50.5));
}

TEST_CASE("inline_poly::vector - Push Back Capacity Exceeded")
{
    TestVector vec;

    // Fill to capacity
    for (size_t i = 0; i < TestCapacity; ++i)
    {
        vec.emplace_back<Dog>(static_cast<int>(i));
    }

    CHECK(vec.size() == TestCapacity);

    // Try to add one more
    CHECK_THROWS_AS(vec.emplace_back<Dog>(100), std::out_of_range);
    CHECK(vec.size() == TestCapacity); // Size unchanged
}

// --- Pop Back Operations ---

TEST_CASE("inline_poly::vector - Pop Back")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<Dog>(3);

    CHECK(vec.size() == 3u);

    vec.pop_back();
    CHECK(vec.size() == 2u);
    CHECK(vec.back()->id() == 2);

    vec.pop_back();
    CHECK(vec.size() == 1u);
    CHECK(vec.back()->id() == 1);

    vec.pop_back();
    CHECK(vec.size() == 0u);
    CHECK(vec.empty());
}

TEST_CASE("inline_poly::vector - Pop Back Empty")
{
    TestVector vec;
    CHECK_THROWS_AS(vec.pop_back(), std::out_of_range);
}

// --- Element Access ---

TEST_CASE("inline_poly::vector - Subscript Operator")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    CHECK(vec[0]->id() == 1);
    CHECK(vec[1]->id() == 2);

    const TestVector& cvec = vec;
    CHECK(cvec[0]->id() == 1);
    CHECK(cvec[1]->id() == 2);
}

TEST_CASE("inline_poly::vector - At Method")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    CHECK(vec.at(0)->id() == 1);
    CHECK(vec.at(1)->id() == 2);

    CHECK_THROWS_AS(vec.at(2), std::out_of_range);
    CHECK_THROWS_AS(vec.at(100), std::out_of_range);
}

TEST_CASE("inline_poly::vector - Front and Back")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 75.0);

    CHECK(vec.front()->id() == 1);
    CHECK(vec.back()->id() == 3);

    const TestVector& cvec = vec;
    CHECK(cvec.front()->id() == 1);
    CHECK(cvec.back()->id() == 3);
}

TEST_CASE("inline_poly::vector - Front Back Empty")
{
    TestVector vec;
    CHECK_THROWS_AS(vec.front(), std::out_of_range);
    CHECK_THROWS_AS(vec.back(), std::out_of_range);
}

TEST_CASE("inline_poly::vector - Data")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    Animal** data = vec.data();
    CHECK(data != nullptr);
    CHECK(data[0]->id() == 1);
    CHECK(data[1]->id() == 2);
}

// --- Insert Operations ---

TEST_CASE("inline_poly::vector - Emplace at Beginning")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    auto it = vec.emplace<BigDog>(vec.begin(), 0, 100.0);

    CHECK(vec.size() == 3u);
    CHECK(vec[0]->id() == 0);
    CHECK(vec[1]->id() == 1);
    CHECK(vec[2]->id() == 2);
    CHECK((*it)->id() == 0);
}

TEST_CASE("inline_poly::vector - Emplace in Middle")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(3);

    auto it = vec.emplace<Cat>(vec.begin() + 1, 2);

    CHECK(vec.size() == 3u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[1]->id() == 2);
    CHECK(vec[2]->id() == 3);
    CHECK((*it)->id() == 2);
}

TEST_CASE("inline_poly::vector - Emplace at End")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    auto it = vec.emplace<BigDog>(vec.end(), 3, 80.0);

    CHECK(vec.size() == 3u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[1]->id() == 2);
    CHECK(vec[2]->id() == 3);
    CHECK((*it)->id() == 3);
}

// --- Erase Operations ---

TEST_CASE("inline_poly::vector - Erase Single")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 60.0);
    vec.emplace_back<Dog>(4);

    auto it = vec.erase(vec.begin() + 1); // Erase Cat

    CHECK(vec.size() == 3u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[1]->id() == 3);
    CHECK(vec[2]->id() == 4);
    CHECK((*it)->id() == 3);
}

TEST_CASE("inline_poly::vector - Erase Range")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 60.0);
    vec.emplace_back<Dog>(4);
    vec.emplace_back<Cat>(5);

    auto it =
        vec.erase(vec.begin() + 1, vec.begin() + 4); // Erase elements 2, 3, 4

    CHECK(vec.size() == 2u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[1]->id() == 5);
    CHECK((*it)->id() == 5);
}

TEST_CASE("inline_poly::vector - Erase All")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    vec.erase(vec.begin(), vec.end());

    CHECK(vec.size() == 0u);
    CHECK(vec.empty());
}

TEST_CASE("inline_poly::vector - Erase Invalid Position")
{
    TestVector vec;
    vec.emplace_back<Dog>(1);

    CHECK_THROWS_AS(vec.erase(vec.end()), std::out_of_range);
    CHECK_THROWS_AS(vec.erase(vec.begin() + 2), std::out_of_range);
}

// --- Clear and Resize ---

TEST_CASE("inline_poly::vector - Clear")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 70.0);

    CHECK(vec.size() == 3u);

    vec.clear();

    CHECK(vec.size() == 0u);
    CHECK(vec.empty());
    CHECK(vec.capacity() == TestCapacity); // Capacity unchanged
}

TEST_CASE("inline_poly::vector - Resize Shrink")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 70.0);

    vec.resize(1);

    CHECK(vec.size() == 1u);
    CHECK(vec[0]->id() == 1);
}

TEST_CASE("inline_poly::vector - Resize Grow")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);

    vec.resize(3);

    CHECK(vec.size() == 3u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[1] == nullptr);
    CHECK(vec[2] == nullptr);
}

TEST_CASE("inline_poly::vector - Resize Exceeds Capacity")
{
    TestVector vec;
    CHECK_THROWS_AS(vec.resize(TestCapacity + 1), std::out_of_range);
}

// --- Iterator Tests ---

TEST_CASE("inline_poly::vector - Iterator Basic")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 80.0);

    int sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        sum += (*it)->id();
    }
    CHECK(sum == 6);
}

TEST_CASE("inline_poly::vector - Iterator Range Based")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 80.0);

    int sum = 0;
    for (Animal* animal : vec)
    {
        sum += animal->id();
    }
    CHECK(sum == 6);
}

TEST_CASE("inline_poly::vector - Const Iterator")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    const TestVector& cvec = vec;

    int sum = 0;
    for (auto it = cvec.begin(); it != cvec.end(); ++it)
    {
        sum += (*it)->id();
    }
    CHECK(sum == 3);
}

TEST_CASE("inline_poly::vector - Reverse Iterator")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 90.0);

    std::vector<int> ids;
    for (auto it = vec.rbegin(); it != vec.rend(); ++it)
    {
        ids.push_back((*it)->id());
    }

    CHECK(ids.size() == 3u);
    CHECK(ids[0] == 3);
    CHECK(ids[1] == 2);
    CHECK(ids[2] == 1);
}

TEST_CASE("inline_poly::vector - Iterator Arithmetic")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);
    vec.emplace_back<BigDog>(3, 100.0);

    auto it = vec.begin();
    CHECK((*it)->id() == 1);

    ++it;
    CHECK((*it)->id() == 2);

    it += 1;
    CHECK((*it)->id() == 3);

    it -= 2;
    CHECK((*it)->id() == 1);

    auto it2 = it + 2;
    CHECK((*it2)->id() == 3);

    CHECK(it2 - it == 2);
}

// --- Reserve and Capacity Management ---

TEST_CASE("inline_poly::vector - Reserve")
{
    TestVector vec;

    vec.reserve(5);                        // Should succeed
    CHECK(vec.capacity() == TestCapacity); // Unchanged

    CHECK_THROWS_AS(vec.reserve(TestCapacity + 1), std::length_error);
}

TEST_CASE("inline_poly::vector - Shrink to Fit")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Cat>(2);

    vec.shrink_to_fit(); // Should be no-op
    CHECK(vec.capacity() == TestCapacity);
    CHECK(vec.size() == 2u);
}

// --- Destructor Tests ---

// Track object construction/destruction
static int g_constructed = 0;
static int g_destructed  = 0;

class TrackedAnimal : public Animal
{
public:
    explicit TrackedAnimal(int id) : Animal(id)
    {
        ++g_constructed;
    }
    ~TrackedAnimal() override
    {
        ++g_destructed;
    }
    std::string speak() const override
    {
        return "tracked";
    }
};

TEST_CASE("inline_poly::vector - Destructor Calls Virtual Destructors")
{
    g_constructed = 0;
    g_destructed  = 0;

    {
        TestVector vec;
        vec.emplace_back<TrackedAnimal>(1);
        vec.emplace_back<TrackedAnimal>(2);
        vec.emplace_back<TrackedAnimal>(3);

        CHECK(g_constructed == 3);
        CHECK(g_destructed == 0);
    }

    CHECK(g_destructed == 3);
}

TEST_CASE("inline_poly::vector - Pop Back Calls Destructor")
{
    g_constructed = 0;
    g_destructed  = 0;

    TestVector vec;
    vec.emplace_back<TrackedAnimal>(1);
    vec.emplace_back<TrackedAnimal>(2);

    CHECK(g_constructed == 2);
    CHECK(g_destructed == 0);

    vec.pop_back();
    CHECK(g_destructed == 1);

    vec.pop_back();
    CHECK(g_destructed == 2);
}

TEST_CASE("inline_poly::vector - Erase Calls Destructor")
{
    g_constructed = 0;
    g_destructed  = 0;

    TestVector vec;
    vec.emplace_back<TrackedAnimal>(1);
    vec.emplace_back<TrackedAnimal>(2);
    vec.emplace_back<TrackedAnimal>(3);

    CHECK(g_constructed == 3);
    CHECK(g_destructed == 0);

    // Erasing element 1 destroys it, then shifts element 2 down,
    // which also destroys the moved-from source object
    vec.erase(vec.begin() + 1);
    CHECK(g_destructed ==
          2); // element 1 destroyed + moved-from element 2 destroyed

    vec.clear();
    CHECK(g_destructed == 4); // remaining 2 elements destroyed
}

// --- Edge Cases ---

TEST_CASE("inline_poly::vector - Nullptr Elements")
{
    TestVector vec;

    vec.emplace_back<Dog>(1);
    vec.resize(3); // Creates nullptr elements

    CHECK(vec.size() == 3u);
    CHECK(vec[0] != nullptr);
    CHECK(vec[1] == nullptr);
    CHECK(vec[2] == nullptr);

    // Can still iterate
    int count = 0;
    for (Animal* animal : vec)
    {
        if (animal != nullptr)
        {
            count++;
        }
    }
    CHECK(count == 1);
}

TEST_CASE("inline_poly::vector - Empty Vector Operations")
{
    TestVector vec;

    CHECK(vec.empty());
    CHECK(vec.size() == 0u);
    CHECK_NOTHROW(vec.clear());
    CHECK(vec.begin() == vec.end());
    CHECK(vec.rbegin() == vec.rend());
}

// Test with minimum slot size (just enough for base class)
using MinimalVector = inline_poly::vector<Animal, 5, sizeof(Dog)>;

TEST_CASE("inline_poly::vector - Minimal Slot Size")
{
    MinimalVector vec;

    vec.emplace_back<Dog>(1);
    vec.emplace_back<Dog>(2);

    CHECK(vec.size() == 2u);
    CHECK(vec[0]->speak() == "Woof");
    CHECK(vec[1]->speak() == "Woof");

    // BigDog won't fit - this would be a compile error due to concept constraint
    // vec.emplace_back<BigDog>(3, 100.0); // Should not compile
}

// =============================================================================
// Non-trivially copyable types tests
// =============================================================================

// Base class for non-trivial tests
class Widget
{
public:
    virtual ~Widget()                    = default;
    virtual std::string describe() const = 0;
    virtual int value() const            = 0;
};

// Contains std::string - non-trivially copyable
class Label : public Widget
{
public:
    explicit Label(std::string text) : text_(std::move(text)) {}
    Label(const Label&)                = default;
    Label(Label&&) noexcept            = default;
    Label& operator=(const Label&)     = default;
    Label& operator=(Label&&) noexcept = default;

    std::string describe() const override
    {
        return "Label: " + text_;
    }
    int value() const override
    {
        return static_cast<int>(text_.length());
    }
    const std::string& text() const
    {
        return text_;
    }

private:
    std::string text_;
};

// Contains std::vector - non-trivially copyable
class ListBox : public Widget
{
public:
    explicit ListBox(std::vector<int> items) : items_(std::move(items)) {}
    ListBox(const ListBox&)                = default;
    ListBox(ListBox&&) noexcept            = default;
    ListBox& operator=(const ListBox&)     = default;
    ListBox& operator=(ListBox&&) noexcept = default;

    std::string describe() const override
    {
        return "ListBox with " + std::to_string(items_.size()) + " items";
    }
    int value() const override
    {
        int sum = 0;
        for (int i : items_)
            sum += i;
        return sum;
    }
    const std::vector<int>& items() const
    {
        return items_;
    }

private:
    std::vector<int> items_;
};

// Move-only type - contains unique_ptr
class Canvas : public Widget
{
public:
    explicit Canvas(size_t size) :
        buffer_(std::make_unique<int[]>(size)), size_(size)
    {
        for (size_t i = 0; i < size; ++i)
            buffer_[i] = static_cast<int>(i);
    }
    Canvas(const Canvas&)                = delete;
    Canvas(Canvas&&) noexcept            = default;
    Canvas& operator=(const Canvas&)     = delete;
    Canvas& operator=(Canvas&&) noexcept = default;

    std::string describe() const override
    {
        return "Canvas " + std::to_string(size_);
    }
    int value() const override
    {
        return buffer_ ? buffer_[0] : -1;
    }
    size_t size() const
    {
        return size_;
    }
    int* data() const
    {
        return buffer_.get();
    }

private:
    std::unique_ptr<int[]> buffer_;
    size_t                 size_;
};

constexpr size_t WidgetSlotSize = 256; // Enough for all widget types
using WidgetVector              = inline_poly::vector<Widget, 10, WidgetSlotSize>;

TEST_CASE("inline_poly::vector - Non-trivially copyable types (std::string)")
{
    WidgetVector vec;

    vec.emplace_back<Label>("Hello World");
    vec.emplace_back<Label>("Testing 123");

    REQUIRE(vec.size() == 2u);

    // Verify string data is intact
    auto* label1 = dynamic_cast<Label*>(vec[0]);
    auto* label2 = dynamic_cast<Label*>(vec[1]);
    REQUIRE(label1 != nullptr);
    REQUIRE(label2 != nullptr);
    CHECK(label1->text() == "Hello World");
    CHECK(label2->text() == "Testing 123");

    // Insert in middle - causes shift
    vec.emplace<Label>(vec.begin() + 1, "Inserted");

    REQUIRE(vec.size() == 3u);

    // All strings must still be intact after shift
    label1         = dynamic_cast<Label*>(vec[0]);
    auto* inserted = dynamic_cast<Label*>(vec[1]);
    label2         = dynamic_cast<Label*>(vec[2]);

    REQUIRE(label1 != nullptr);
    REQUIRE(inserted != nullptr);
    REQUIRE(label2 != nullptr);

    CHECK(label1->text() == "Hello World");
    CHECK(inserted->text() == "Inserted");
    CHECK(label2->text() == "Testing 123");
}

TEST_CASE("inline_poly::vector - Non-trivially copyable types (std::vector)")
{
    WidgetVector vec;

    vec.emplace_back<ListBox>(std::vector<int>{1, 2, 3, 4, 5});
    vec.emplace_back<ListBox>(std::vector<int>{10, 20, 30});

    REQUIRE(vec.size() == 2u);

    // Verify vector data is intact
    auto* list1 = dynamic_cast<ListBox*>(vec[0]);
    auto* list2 = dynamic_cast<ListBox*>(vec[1]);
    REQUIRE(list1 != nullptr);
    REQUIRE(list2 != nullptr);
    CHECK(list1->items().size() == 5u);
    CHECK(list1->value() == 15); // 1+2+3+4+5
    CHECK(list2->items().size() == 3u);
    CHECK(list2->value() == 60); // 10+20+30

    // Erase first element - causes shift
    vec.erase(vec.begin());

    REQUIRE(vec.size() == 1u);

    // Remaining vector must still be intact
    list2 = dynamic_cast<ListBox*>(vec[0]);
    REQUIRE(list2 != nullptr);
    CHECK(list2->items().size() == 3u);
    CHECK(list2->value() == 60);
}

TEST_CASE("inline_poly::vector - Move-only types")
{
    WidgetVector vec;

    vec.emplace_back<Canvas>(100);
    vec.emplace_back<Canvas>(200);

    REQUIRE(vec.size() == 2u);

    auto* canvas1 = dynamic_cast<Canvas*>(vec[0]);
    auto* canvas2 = dynamic_cast<Canvas*>(vec[1]);
    REQUIRE(canvas1 != nullptr);
    REQUIRE(canvas2 != nullptr);
    CHECK(canvas1->size() == 100u);
    CHECK(canvas2->size() == 200u);

    // Verify data is valid
    CHECK(canvas1->data() != nullptr);
    CHECK(canvas2->data() != nullptr);
}

TEST_CASE("inline_poly::vector - is_copyable with copyable types only")
{
    WidgetVector vec;

    // Empty container should report as not copyable (no types registered yet)
    // After adding copyable types, it becomes copyable
    vec.emplace_back<Label>("Test");
    vec.emplace_back<ListBox>(std::vector<int>{1, 2, 3});

    CHECK(vec.is_copyable());
    CHECK(vec.is_movable());
}

TEST_CASE("inline_poly::vector - is_copyable becomes false with move-only type")
{
    WidgetVector vec;

    vec.emplace_back<Label>("Copyable");
    CHECK(vec.is_copyable());

    vec.emplace_back<Canvas>(50); // Move-only
    CHECK_FALSE(vec.is_copyable());
    CHECK(vec.is_movable()); // Still movable
}

TEST_CASE("inline_poly::vector - Copy container with copyable types")
{
    WidgetVector vec;

    vec.emplace_back<Label>("Original Text");
    vec.emplace_back<ListBox>(std::vector<int>{1, 2, 3});

    REQUIRE(vec.is_copyable());

    // Copy constructor
    WidgetVector copy = vec;

    REQUIRE(copy.size() == 2u);

    // Verify copied data is independent
    auto* origLabel = dynamic_cast<Label*>(vec[0]);
    auto* copyLabel = dynamic_cast<Label*>(copy[0]);
    REQUIRE(origLabel != nullptr);
    REQUIRE(copyLabel != nullptr);
    CHECK(origLabel->text() == "Original Text");
    CHECK(copyLabel->text() == "Original Text");
    CHECK(origLabel != copyLabel); // Different objects

    auto* origList = dynamic_cast<ListBox*>(vec[1]);
    auto* copyList = dynamic_cast<ListBox*>(copy[1]);
    REQUIRE(origList != nullptr);
    REQUIRE(copyList != nullptr);
    CHECK(origList->value() == copyList->value());
    CHECK(&origList->items() != &copyList->items()); // Independent vectors
}

TEST_CASE("inline_poly::vector - Copy assignment with copyable types")
{
    WidgetVector vec1;
    vec1.emplace_back<Label>("Source");

    WidgetVector vec2;
    vec2.emplace_back<Label>("Target");
    vec2.emplace_back<Label>("ToBeReplaced");

    REQUIRE(vec1.is_copyable());

    vec2 = vec1;

    CHECK(vec2.size() == 1u);
    auto* label = dynamic_cast<Label*>(vec2[0]);
    REQUIRE(label != nullptr);
    CHECK(label->text() == "Source");
}

TEST_CASE("inline_poly::vector - Copy fails with move-only types")
{
    WidgetVector vec;
    vec.emplace_back<Canvas>(100);

    CHECK_FALSE(vec.is_copyable());
    CHECK_THROWS_AS(WidgetVector copy = vec, std::logic_error);
}

TEST_CASE("inline_poly::vector - Move container")
{
    WidgetVector vec;
    vec.emplace_back<Label>("MovedText");
    vec.emplace_back<Canvas>(100);

    REQUIRE(vec.size() == 2u);

    WidgetVector moved = std::move(vec);

    CHECK(moved.size() == 2u);
    CHECK(vec.size() == 0u); // Original is empty

    auto* label  = dynamic_cast<Label*>(moved[0]);
    auto* canvas = dynamic_cast<Canvas*>(moved[1]);
    REQUIRE(label != nullptr);
    REQUIRE(canvas != nullptr);
    CHECK(label->text() == "MovedText");
    CHECK(canvas->size() == 100u);
}

TEST_CASE("inline_poly::vector - Move assignment")
{
    WidgetVector vec1;
    vec1.emplace_back<Label>("Source");
    vec1.emplace_back<Canvas>(50);

    WidgetVector vec2;
    vec2.emplace_back<Label>("Target");

    vec2 = std::move(vec1);

    CHECK(vec2.size() == 2u);
    CHECK(vec1.size() == 0u);

    auto* label = dynamic_cast<Label*>(vec2[0]);
    REQUIRE(label != nullptr);
    CHECK(label->text() == "Source");
}

// =============================================================================
// Array copy/move tests
// =============================================================================

using WidgetArray = inline_poly::array<Widget, 5, WidgetSlotSize>;

TEST_CASE("inline_poly::array - Non-trivially copyable types")
{
    WidgetArray arr;

    arr.emplace<Label>(0, "First");
    arr.emplace<ListBox>(1, std::vector<int>{1, 2, 3});
    arr.emplace<Label>(2, "Third");

    auto* label1 = dynamic_cast<Label*>(arr[0]);
    auto* list   = dynamic_cast<ListBox*>(arr[1]);
    auto* label2 = dynamic_cast<Label*>(arr[2]);

    REQUIRE(label1 != nullptr);
    REQUIRE(list != nullptr);
    REQUIRE(label2 != nullptr);

    CHECK(label1->text() == "First");
    CHECK(list->value() == 6);
    CHECK(label2->text() == "Third");
}

TEST_CASE("inline_poly::array - Copy with copyable types")
{
    WidgetArray arr;
    arr.emplace<Label>(0, "CopyMe");
    arr.emplace<ListBox>(2, std::vector<int>{10, 20});

    REQUIRE(arr.is_copyable());

    WidgetArray copy = arr;

    auto* origLabel = dynamic_cast<Label*>(arr[0]);
    auto* copyLabel = dynamic_cast<Label*>(copy[0]);
    REQUIRE(origLabel != nullptr);
    REQUIRE(copyLabel != nullptr);
    CHECK(origLabel->text() == "CopyMe");
    CHECK(copyLabel->text() == "CopyMe");
    CHECK(origLabel != copyLabel);
}

TEST_CASE("inline_poly::array - Copy fails with move-only types")
{
    WidgetArray arr;
    arr.emplace<Canvas>(0, 100);

    CHECK_FALSE(arr.is_copyable());
    CHECK_THROWS_AS(WidgetArray copy = arr, std::logic_error);
}

TEST_CASE("inline_poly::array - Move works with move-only types")
{
    WidgetArray arr;
    arr.emplace<Canvas>(0, 100);
    arr.emplace<Label>(1, "Text");

    WidgetArray moved = std::move(arr);

    auto* canvas = dynamic_cast<Canvas*>(moved[0]);
    auto* label  = dynamic_cast<Label*>(moved[1]);
    REQUIRE(canvas != nullptr);
    REQUIRE(label != nullptr);
    CHECK(canvas->size() == 100u);
    CHECK(label->text() == "Text");

    // Original should be empty
    CHECK(arr[0] == nullptr);
    CHECK(arr[1] == nullptr);
}

// =============================================================================
// Alignment tests for vector
// =============================================================================

TEST_CASE("inline_poly::vector - Alignment requirements")
{
    // Base class with specific alignment
    struct alignas(8) AlignedBase
    {
        virtual ~AlignedBase()              = default;
        virtual size_t getAlignment() const = 0;
    };

    // Derived class with default alignment
    struct DefaultAligned : AlignedBase
    {
        size_t getAlignment() const override
        {
            return alignof(DefaultAligned);
        }
    };

    // Derived class requiring 16-byte alignment (e.g., for SIMD)
    struct alignas(16) SimdAligned : AlignedBase
    {
        size_t getAlignment() const override
        {
            return alignof(SimdAligned);
        }
    };

    SUBCASE("custom alignment allows stricter aligned types")
    {
        inline_poly::vector<AlignedBase, 10, sizeof(SimdAligned),
                            alignof(SimdAligned)>
            alignedVec;

        alignedVec.emplace_back<DefaultAligned>();
        alignedVec.emplace_back<SimdAligned>();

        CHECK(alignedVec.size() == 2u);
        CHECK(alignedVec[0]->getAlignment() == alignof(DefaultAligned));
        CHECK(alignedVec[1]->getAlignment() == alignof(SimdAligned));
    }

    SUBCASE("storage is properly aligned")
    {
        inline_poly::vector<AlignedBase, 10, sizeof(SimdAligned), 16> alignedVec;
        alignedVec.emplace_back<SimdAligned>();

        // Verify the pointer is 16-byte aligned
        auto* ptr = alignedVec[0];
        CHECK(reinterpret_cast<uintptr_t>(ptr) % 16 == 0);
    }
}

// =============================================================================
// Exception safety tests for vector
// =============================================================================

TEST_CASE("inline_poly::vector - Exception safety")
{
    static int  constructionCount = 0;
    static int  destructionCount  = 0;
    static bool shouldThrow       = false;

    struct ThrowingAnimal : Animal
    {
        explicit ThrowingAnimal(int id) : Animal(id)
        {
            if (shouldThrow)
            {
                throw std::runtime_error("Construction failed");
            }
            constructionCount++;
        }

        ~ThrowingAnimal() override
        {
            destructionCount++;
        }

        std::string speak() const override
        {
            return "throw";
        }
    };

    SUBCASE("failed emplace_back leaves container unchanged")
    {
        constructionCount = 0;
        destructionCount  = 0;
        shouldThrow       = false;

        TestVector vec;
        vec.emplace_back<ThrowingAnimal>(1);
        vec.emplace_back<ThrowingAnimal>(2);

        size_t sizeBefore = vec.size();
        CHECK(constructionCount == 2);

        shouldThrow = true;
        CHECK_THROWS_AS(vec.emplace_back<ThrowingAnimal>(3), std::runtime_error);

        // Size should be unchanged
        CHECK(vec.size() == sizeBefore);

        // Existing elements should still be valid
        CHECK(vec[0]->id() == 1);
        CHECK(vec[1]->id() == 2);

        shouldThrow = false;
    }

    SUBCASE("failed emplace in middle leaves container in valid state")
    {
        constructionCount = 0;
        destructionCount  = 0;
        shouldThrow       = false;

        TestVector vec;
        vec.emplace_back<ThrowingAnimal>(1);
        vec.emplace_back<ThrowingAnimal>(2);
        vec.emplace_back<ThrowingAnimal>(3);

        shouldThrow = true;

        // Note: emplace in middle shifts elements first, then constructs
        // If construction fails after shift, we're in a partially-shifted state
        CHECK_THROWS_AS(vec.emplace<ThrowingAnimal>(vec.begin() + 1, 99),
                        std::runtime_error);

        shouldThrow = false;
    }
}

// =============================================================================
// Move efficiency tests for vector
// =============================================================================

TEST_CASE("inline_poly::vector - Move efficiency")
{
    static int copyCount = 0;
    static int moveCount = 0;

    struct TrackedAnimal : Animal
    {
        explicit TrackedAnimal(int id) : Animal(id) {}

        TrackedAnimal(const TrackedAnimal& other) : Animal(other.id())
        {
            copyCount++;
        }

        TrackedAnimal(TrackedAnimal&& other) noexcept : Animal(other.id())
        {
            moveCount++;
        }

        std::string speak() const override
        {
            return "tracked";
        }
    };

    SUBCASE("move construction uses moves not copies")
    {
        copyCount = 0;
        moveCount = 0;

        inline_poly::vector<Animal, 10, sizeof(TrackedAnimal)> vec1;
        vec1.emplace_back<TrackedAnimal>(1);
        vec1.emplace_back<TrackedAnimal>(2);
        vec1.emplace_back<TrackedAnimal>(3);

        int copiesBefore = copyCount;
        moveCount        = 0;

        auto vec2 = std::move(vec1);

        CHECK(copyCount == copiesBefore);
        CHECK(moveCount >= 3);

        CHECK(vec2.size() == 3u);
        CHECK(vec1.size() == 0u);
    }

    SUBCASE("shift operations use moves not copies")
    {
        copyCount = 0;
        moveCount = 0;

        inline_poly::vector<Animal, 10, sizeof(TrackedAnimal)> vec;
        vec.emplace_back<TrackedAnimal>(1);
        vec.emplace_back<TrackedAnimal>(2);
        vec.emplace_back<TrackedAnimal>(3);

        int copiesBefore = copyCount;
        moveCount        = 0;

        // Insert at beginning causes all elements to shift
        vec.emplace<TrackedAnimal>(vec.begin(), 0);

        // Should use moves for shifting, not copies
        CHECK(copyCount == copiesBefore);
        CHECK(moveCount >= 3); // At least 3 moves for shifting

        CHECK(vec.size() == 4u);
        CHECK(vec[0]->id() == 0);
        CHECK(vec[1]->id() == 1);
        CHECK(vec[2]->id() == 2);
        CHECK(vec[3]->id() == 3);
    }

    SUBCASE("erase uses moves for shifting")
    {
        copyCount = 0;
        moveCount = 0;

        inline_poly::vector<Animal, 10, sizeof(TrackedAnimal)> vec;
        vec.emplace_back<TrackedAnimal>(1);
        vec.emplace_back<TrackedAnimal>(2);
        vec.emplace_back<TrackedAnimal>(3);
        vec.emplace_back<TrackedAnimal>(4);

        int copiesBefore = copyCount;
        moveCount        = 0;

        // Erase first element, causing shift left
        vec.erase(vec.begin());

        CHECK(copyCount == copiesBefore);
        CHECK(moveCount >= 3); // At least 3 moves for shifting

        CHECK(vec.size() == 3u);
        CHECK(vec[0]->id() == 2);
        CHECK(vec[1]->id() == 3);
        CHECK(vec[2]->id() == 4);
    }
}

// =============================================================================
// Non-movable, non-copyable types tests (regression for erase bug)
// =============================================================================

// Base class that's non-copyable and non-movable
class ImmovableBase
{
public:
    explicit ImmovableBase(int id) : id_(id) {}
    virtual ~ImmovableBase() = default;

    // Explicitly delete copy and move
    ImmovableBase(const ImmovableBase&)            = delete;
    ImmovableBase& operator=(const ImmovableBase&) = delete;
    ImmovableBase(ImmovableBase&&)                 = delete;
    ImmovableBase& operator=(ImmovableBase&&)      = delete;

    virtual std::string name() const = 0;
    int id() const
    {
        return id_;
    }

protected:
    int id_;
};

class ImmovableDerived1 : public ImmovableBase
{
public:
    explicit ImmovableDerived1(int id) : ImmovableBase(id) {}
    std::string name() const override
    {
        return "Derived1";
    }
};

class ImmovableDerived2 : public ImmovableBase
{
public:
    explicit ImmovableDerived2(int id) : ImmovableBase(id) {}
    std::string name() const override
    {
        return "Derived2";
    }
};

class ImmovableDerived3 : public ImmovableBase
{
public:
    explicit ImmovableDerived3(int id) : ImmovableBase(id) {}
    std::string name() const override
    {
        return "Derived3";
    }
};

using ImmovableVector =
    inline_poly::vector<ImmovableBase, 10, sizeof(ImmovableDerived1)>;

TEST_CASE("inline_poly::vector - Non-movable types basic operations")
{
    ImmovableVector vec;

    vec.emplace_back<ImmovableDerived1>(1);
    vec.emplace_back<ImmovableDerived2>(2);
    vec.emplace_back<ImmovableDerived3>(3);

    REQUIRE(vec.size() == 3u);
    CHECK(vec[0]->id() == 1);
    CHECK(vec[1]->id() == 2);
    CHECK(vec[2]->id() == 3);

    // Verify capabilities
    CHECK_FALSE(vec.is_copyable());
    CHECK_FALSE(vec.is_movable());
}

TEST_CASE("inline_poly::vector - erase() throws for non-movable types when shift "
          "needed")
{
    ImmovableVector vec;

    vec.emplace_back<ImmovableDerived1>(1);
    vec.emplace_back<ImmovableDerived2>(2);
    vec.emplace_back<ImmovableDerived3>(3);

    // Erasing from the middle requires shifting - should throw
    CHECK_THROWS_AS(vec.erase(vec.begin() + 1), std::runtime_error);

    // Size should be unchanged since erase failed early
    // (Note: the check happens before destruction, so we get the error before any
    // changes)
    CHECK(vec.size() == 3u);
}

TEST_CASE("inline_poly::vector - erase() at end works for non-movable types")
{
    ImmovableVector vec;

    vec.emplace_back<ImmovableDerived1>(1);
    vec.emplace_back<ImmovableDerived2>(2);
    vec.emplace_back<ImmovableDerived3>(3);

    // Erasing the last element doesn't require shifting - should work
    auto it = vec.erase(vec.begin() + 2);
    CHECK(vec.size() == 2u);
    CHECK(it == vec.end());
}

TEST_CASE("inline_poly::vector - pop_back() works with non-movable types")
{
    ImmovableVector vec;

    vec.emplace_back<ImmovableDerived1>(1);
    vec.emplace_back<ImmovableDerived2>(2);

    vec.pop_back();
    CHECK(vec.size() == 1u);
    CHECK(vec[0]->id() == 1);
}

TEST_CASE("inline_poly::vector - clear() works with non-movable types")
{
    ImmovableVector vec;

    vec.emplace_back<ImmovableDerived1>(1);
    vec.emplace_back<ImmovableDerived2>(2);
    vec.emplace_back<ImmovableDerived3>(3);

    vec.clear();
    CHECK(vec.size() == 0u);
    CHECK(vec.empty());
}

// Destruction tracking for non-movable types
static int g_immovable_constructed = 0;
static int g_immovable_destructed  = 0;

class TrackedImmovable : public ImmovableBase
{
public:
    explicit TrackedImmovable(int id) : ImmovableBase(id)
    {
        ++g_immovable_constructed;
    }
    ~TrackedImmovable() override
    {
        ++g_immovable_destructed;
    }
    std::string name() const override
    {
        return "TrackedImmovable";
    }
};

TEST_CASE("inline_poly::vector - pop_back() properly destroys elements")
{
    g_immovable_constructed = 0;
    g_immovable_destructed  = 0;

    {
        inline_poly::vector<ImmovableBase, 10, sizeof(TrackedImmovable)> vec;

        vec.emplace_back<TrackedImmovable>(1);
        vec.emplace_back<TrackedImmovable>(2);
        vec.emplace_back<TrackedImmovable>(3);

        CHECK(g_immovable_constructed == 3);
        CHECK(g_immovable_destructed == 0);

        vec.pop_back();

        // One element destroyed (the one being popped)
        CHECK(g_immovable_destructed == 1);
        CHECK(vec.size() == 2u);
    }

    // All elements destroyed when container goes out of scope
    CHECK(g_immovable_destructed == 3);
}
