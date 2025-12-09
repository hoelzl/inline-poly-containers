// Copyright 2025 Dr. Matthias Hölzl
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../include/inline_poly.h"

// Test fixture base class
struct Animal
{
    virtual ~Animal()         = default;
    virtual int speak() const = 0;
};

struct Dog : Animal
{
    int barkCount;
    explicit Dog(int count = 1) : barkCount(count) {}
    int speak() const override
    {
        return barkCount;
    }
};

struct Cat : Animal
{
    int speak() const override
    {
        return -1;
    }
};

struct LargeDog : Animal
{
    int data[10]{};
    explicit LargeDog(int val = 0)
    {
        data[0] = val;
    }
    int speak() const override
    {
        return data[0];
    }
};

TEST_CASE("inline_poly::array basic operations")
{
    inline_poly::array<Animal, 4, sizeof(LargeDog)> animals;

    SUBCASE("initial state has null pointers")
    {
        CHECK(animals[0] == nullptr);
        CHECK(animals[1] == nullptr);
        CHECK(animals[2] == nullptr);
        CHECK(animals[3] == nullptr);
    }

    SUBCASE("emplace creates object and returns pointer")
    {
        Dog* dog = animals.emplace<Dog>(0, 3);
        CHECK(dog != nullptr);
        CHECK(dog->barkCount == 3);
        CHECK(animals[0] != nullptr);
        CHECK(animals[0]->speak() == 3);
    }

    SUBCASE("emplace different derived types")
    {
        animals.emplace<Dog>(0, 5);
        animals.emplace<Cat>(1);
        animals.emplace<LargeDog>(2, 42);

        CHECK(animals[0]->speak() == 5);
        CHECK(animals[1]->speak() == -1);
        CHECK(animals[2]->speak() == 42);
    }

    SUBCASE("emplace overwrites existing object")
    {
        animals.emplace<Dog>(0, 1);
        CHECK(animals[0]->speak() == 1);

        animals.emplace<Cat>(0);
        CHECK(animals[0]->speak() == -1);
    }

    SUBCASE("clear destroys all objects")
    {
        animals.emplace<Dog>(0);
        animals.emplace<Cat>(1);
        animals.emplace<LargeDog>(2);

        animals.clear();

        CHECK(animals[0] == nullptr);
        CHECK(animals[1] == nullptr);
        CHECK(animals[2] == nullptr);
    }
}

TEST_CASE("inline_poly::array element access")
{
    inline_poly::array<Animal, 4, sizeof(Dog)> animals;
    animals.emplace<Dog>(0, 10);
    animals.emplace<Cat>(3);

    SUBCASE("operator[] provides access")
    {
        CHECK(animals[0]->speak() == 10);
        CHECK(animals[3]->speak() == -1);
    }

    SUBCASE("at() provides checked access")
    {
        CHECK(animals.at(0)->speak() == 10);
        CHECK(animals.at(3)->speak() == -1);
    }

    SUBCASE("at() throws on out of range")
    {
        CHECK_THROWS_AS(animals.at(4), std::out_of_range);
        CHECK_THROWS_AS(animals.at(100), std::out_of_range);
    }

    SUBCASE("front() returns first element")
    {
        CHECK(animals.front() != nullptr);
        CHECK(animals.front()->speak() == 10);
    }

    SUBCASE("back() returns last element")
    {
        CHECK(animals.back() != nullptr);
        CHECK(animals.back()->speak() == -1);
    }

    SUBCASE("data() returns pointer to internal array")
    {
        Animal** ptr = animals.data();
        CHECK(ptr != nullptr);
        CHECK(ptr[0]->speak() == 10);
    }
}

TEST_CASE("inline_poly::array capacity")
{
    inline_poly::array<Animal, 5, sizeof(Dog)> animals;

    SUBCASE("size() returns template parameter N")
    {
        CHECK(animals.size() == 5);
    }

    SUBCASE("max_size() returns template parameter N")
    {
        CHECK(animals.max_size() == 5);
    }

    SUBCASE("empty() returns false for non-zero size")
    {
        CHECK_FALSE(animals.empty());
    }
}

TEST_CASE("inline_poly::array iterators")
{
    inline_poly::array<Animal, 3, sizeof(Dog)> animals;

    SUBCASE("range-based for loop works")
    {
        animals.emplace<Dog>(0, 1);
        animals.emplace<Cat>(1);

        int count = 0;
        for (auto* ptr : animals)
        {
            if (ptr != nullptr) count++;
        }
        CHECK(count == 2);
    }

    SUBCASE("begin/end iteration")
    {
        animals.emplace<Dog>(0, 1);
        animals.emplace<Dog>(1, 2);
        animals.emplace<Dog>(2, 3);

        int sum = 0;
        for (auto it = animals.begin(); it != animals.end(); ++it)
        {
            if (*it != nullptr) sum += (*it)->speak();
        }
        CHECK(sum == 6);
    }

    SUBCASE("const iteration")
    {
        animals.emplace<Dog>(0, 10);

        const auto& constAnimals = animals;
        int         sum          = 0;
        for (auto it = constAnimals.cbegin(); it != constAnimals.cend(); ++it)
        {
            if (*it != nullptr) sum += (*it)->speak();
        }
        CHECK(sum == 10);
    }

    SUBCASE("reverse iteration")
    {
        animals.emplace<Dog>(0, 1);
        animals.emplace<Dog>(1, 2);
        animals.emplace<Dog>(2, 3);

        std::vector<int> values;
        for (auto it = animals.rbegin(); it != animals.rend(); ++it)
        {
            if (*it != nullptr) values.push_back((*it)->speak());
        }

        CHECK(values.size() == 3);
        CHECK(values[0] == 3);
        CHECK(values[1] == 2);
        CHECK(values[2] == 1);
    }
}

TEST_CASE("inline_poly::array destructor cleanup")
{
    static int destructorCount = 0;

    struct Tracked : Animal
    {
        ~Tracked() override
        {
            destructorCount++;
        }
        int speak() const override
        {
            return 0;
        }
    };

    destructorCount = 0;

    {
        inline_poly::array<Animal, 3, sizeof(Tracked)> arr;
        arr.emplace<Tracked>(0);
        arr.emplace<Tracked>(1);
        arr.emplace<Tracked>(2);
    }

    CHECK(destructorCount == 3);
}

TEST_CASE("inline_poly::array const correctness")
{
    inline_poly::array<Animal, 2, sizeof(Dog)> animals;
    animals.emplace<Dog>(0, 5);

    const auto& constRef = animals;

    SUBCASE("const operator[]")
    {
        CHECK(constRef[0]->speak() == 5);
    }

    SUBCASE("const at()")
    {
        CHECK(constRef.at(0)->speak() == 5);
    }

    SUBCASE("const front/back")
    {
        CHECK(constRef.front()->speak() == 5);
    }

    SUBCASE("const data()")
    {
        Animal* const* ptr = constRef.data();
        CHECK(ptr[0]->speak() == 5);
    }
}

// =============================================================================
// Alignment tests
// =============================================================================

TEST_CASE("inline_poly::array alignment requirements")
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

    SUBCASE("default alignment works for base-aligned types")
    {
        inline_poly::array<AlignedBase, 4, sizeof(DefaultAligned)> arr;
        arr.emplace<DefaultAligned>(0);

        CHECK(arr[0] != nullptr);
        CHECK(arr[0]->getAlignment() == alignof(DefaultAligned));
    }

    SUBCASE("custom alignment allows stricter aligned types")
    {
        // Array with 16-byte alignment to accommodate SimdAligned
        inline_poly::array<AlignedBase, 4, sizeof(SimdAligned),
                           alignof(SimdAligned)>
            alignedArray;

        alignedArray.emplace<DefaultAligned>(0);
        alignedArray.emplace<SimdAligned>(1);

        CHECK(alignedArray[0] != nullptr);
        CHECK(alignedArray[1] != nullptr);
        CHECK(alignedArray[0]->getAlignment() == alignof(DefaultAligned));
        CHECK(alignedArray[1]->getAlignment() == alignof(SimdAligned));
    }

    SUBCASE("storage is properly aligned")
    {
        inline_poly::array<AlignedBase, 4, sizeof(SimdAligned), 16> alignedArray;
        alignedArray.emplace<SimdAligned>(0);

        // Verify the pointer is 16-byte aligned
        auto* ptr = alignedArray[0];
        CHECK(reinterpret_cast<uintptr_t>(ptr) % 16 == 0);
    }

    // Note: The following would fail to compile due to FitsInSlot concept:
    // inline_poly::array<AlignedBase, 4, sizeof(SimdAligned)> nonAlignedArray;
    // nonAlignedArray.emplace<SimdAligned>(0);  // Compile error: alignment not
    // satisfied
}

// =============================================================================
// Exception safety tests
// =============================================================================

TEST_CASE("inline_poly::array exception safety")
{
    static int  constructionCount = 0;
    static int  destructionCount  = 0;
    static bool shouldThrow       = false;

    struct ThrowingType : Animal
    {
        int value;

        explicit ThrowingType(int v) : value(v)
        {
            if (shouldThrow)
            {
                throw std::runtime_error("Construction failed");
            }
            constructionCount++;
        }

        ~ThrowingType() override
        {
            destructionCount++;
        }

        int speak() const override
        {
            return value;
        }
    };

    SUBCASE("failed construction leaves container unchanged")
    {
        constructionCount = 0;
        destructionCount  = 0;
        shouldThrow       = false;

        inline_poly::array<Animal, 4, sizeof(ThrowingType)> arr;

        // Successfully add some elements
        arr.emplace<ThrowingType>(0, 1);
        arr.emplace<ThrowingType>(1, 2);
        CHECK(constructionCount == 2);

        // Now make construction throw
        shouldThrow = true;

        CHECK_THROWS_AS(arr.emplace<ThrowingType>(2, 3), std::runtime_error);

        // Existing elements should still be valid
        CHECK(arr[0] != nullptr);
        CHECK(arr[1] != nullptr);
        CHECK(arr[0]->speak() == 1);
        CHECK(arr[1]->speak() == 2);

        // The failed slot should remain unchanged (was nullptr)
        CHECK(arr[2] == nullptr);

        shouldThrow = false;
    }

    SUBCASE("failed replacement restores previous state")
    {
        constructionCount = 0;
        destructionCount  = 0;
        shouldThrow       = false;

        inline_poly::array<Animal, 4, sizeof(ThrowingType)> arr;
        arr.emplace<ThrowingType>(0, 100);

        CHECK(arr[0]->speak() == 100);
        int destructionsBefore = destructionCount;

        // Try to replace with throwing construction
        shouldThrow = true;
        CHECK_THROWS_AS(arr.emplace<ThrowingType>(0, 200), std::runtime_error);

        // The old element was destroyed before the throw
        CHECK(destructionCount == destructionsBefore + 1);

        // Slot is now empty due to the failure
        CHECK(arr[0] == nullptr);

        shouldThrow = false;
    }
}

// =============================================================================
// Move efficiency tests
// =============================================================================

TEST_CASE("inline_poly::array move efficiency")
{
    static int copyCount = 0;
    static int moveCount = 0;

    struct TrackedMove : Animal
    {
        int value;

        explicit TrackedMove(int v) : value(v) {}

        TrackedMove(const TrackedMove& other) : value(other.value)
        {
            copyCount++;
        }

        TrackedMove(TrackedMove&& other) noexcept : value(other.value)
        {
            moveCount++;
        }

        int speak() const override
        {
            return value;
        }
    };

    SUBCASE("move construction uses moves not copies")
    {
        copyCount = 0;
        moveCount = 0;

        inline_poly::array<Animal, 4, sizeof(TrackedMove)> arr1;
        arr1.emplace<TrackedMove>(0, 1);
        arr1.emplace<TrackedMove>(1, 2);
        arr1.emplace<TrackedMove>(2, 3);

        int copiesBefore = copyCount;

        auto arr2 = std::move(arr1);

        // Should use moves, not copies
        CHECK(copyCount == copiesBefore);
        CHECK(moveCount >= 3);

        // Moved-to array has the values
        CHECK(arr2[0]->speak() == 1);
        CHECK(arr2[1]->speak() == 2);
        CHECK(arr2[2]->speak() == 3);

        // Source array is empty
        CHECK(arr1[0] == nullptr);
        CHECK(arr1[1] == nullptr);
        CHECK(arr1[2] == nullptr);
    }

    SUBCASE("move assignment uses moves not copies")
    {
        copyCount = 0;
        moveCount = 0;

        inline_poly::array<Animal, 4, sizeof(TrackedMove)> arr1;
        arr1.emplace<TrackedMove>(0, 10);
        arr1.emplace<TrackedMove>(1, 20);

        inline_poly::array<Animal, 4, sizeof(TrackedMove)> arr2;

        int copiesBefore = copyCount;
        moveCount        = 0;

        arr2 = std::move(arr1);

        CHECK(copyCount == copiesBefore);
        CHECK(moveCount >= 2);

        CHECK(arr2[0]->speak() == 10);
        CHECK(arr2[1]->speak() == 20);
    }
}
