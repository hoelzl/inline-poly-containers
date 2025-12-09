// Copyright 2025 Dr. Matthias Hölzl
#include <cstddef>
#include <iostream>
#include "../../include/inline_poly.h"

// Base class
struct alignas(8) AlignedBase
{
    virtual ~AlignedBase()                            = default;
    [[nodiscard]] virtual size_t getAlignment() const = 0;
};

// Derived class with default alignment
struct DefaultAligned : AlignedBase
{
    [[nodiscard]] size_t getAlignment() const override
    {
        return alignof(DefaultAligned);
    }
};

// Derived class requiring 16-byte alignment (e.g., for SIMD)
struct alignas(16) SimdAligned : AlignedBase
{
    [[nodiscard]] size_t getAlignment() const override
    {
        return alignof(SimdAligned);
    }
};

int main()
{
    std::cout << "Testing alignment requirements:\n";
    std::cout << "Base alignment: " << alignof(AlignedBase) << " bytes\n";
    std::cout << "DefaultAligned alignment: " << alignof(DefaultAligned)
              << " bytes\n";
    std::cout << "SimdAligned alignment: " << alignof(SimdAligned)
              << " bytes\n\n";

    // Array with 8-byte alignment. Cannot store SimdAligned
    inline_poly::poly_array<AlignedBase, 4, sizeof(SimdAligned)> nonAlignedArray;

    nonAlignedArray.emplace<DefaultAligned>(0);

    // Fails to compile with the following error, as it should:
    //
    // Substitution failed due to constraints:
    // FitsInSlot<SimdAligned, AlignedBase, 16, 8> is false, because requirement
    // alignof(SimdAligned) <= 8 is not satisfied
    //
    // nonAlignedArray.emplace<SimdAligned>(1);

    // Use type_list to automatically compute the required size and alignment
    using AlignedTypes = inline_poly::type_list<DefaultAligned, SimdAligned>;
    using Config       = inline_poly::slot_config<AlignedTypes>;

    std::cout << "Using slot_config:\n";
    std::cout << "  Computed size: " << Config::size << " bytes\n";
    std::cout << "  Computed alignment: " << Config::alignment << " bytes\n\n";

    // Array with proper alignment to accommodate all types
    inline_poly::poly_array<AlignedBase, 4, Config::size, Config::alignment>
        alignedArray;

    // This will work because 16-byte alignment satisfies both types
    alignedArray.emplace<DefaultAligned>(0);
    alignedArray.emplace<SimdAligned>(1);

    std::cout << "Successfully created array with custom alignment.\n";
    std::cout << "Element 0 alignment requirement: "
              << alignedArray[0]->getAlignment() << " bytes\n";
    std::cout << "Element 1 alignment requirement: "
              << alignedArray[1]->getAlignment() << " bytes\n";

    // Verify the storage is actually aligned
    auto* storage = reinterpret_cast<std::byte*>(&alignedArray);
    std::cout << "\nStorage address alignment check:\n";
    std::cout << "Storage address: " << static_cast<void*>(storage) << "\n";
    std::cout << "Is 16-byte aligned: "
              << (reinterpret_cast<uintptr_t>(storage) % 16 == 0 ? "yes" : "no")
              << "\n";

    return 0;
}
