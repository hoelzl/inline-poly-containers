// Copyright 2025 Dr. Matthias Hölzl

// inline_poly.h - Type-safe polymorphic containers with inline storage
// ReSharper disable CppMemberFunctionMayBeStatic

#pragma once
#ifndef INLINE_POLY_H
#define INLINE_POLY_H

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <format>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace inline_poly
{

    // =============================================================================
    // Type Operations - Type-erased operations for safe polymorphic manipulation
    // =============================================================================

    // Type-erased operations for a specific type
    struct type_operations
    {
        // Destructor function
        using destructor_fn = void (*)(void* obj) noexcept;

        // Move constructor function (constructs dst from src, leaves src in valid
        // but unspecified state)
        using move_constructor_fn = void (*)(void* dst, void* src) noexcept;

        // Move assignment function (assigns dst from src, destroys old dst,
        // leaves src valid but unspecified)
        using move_assignment_fn = void (*)(void* dst, void* src) noexcept;

        // Copy constructor function (constructs dst from src)
        using copy_constructor_fn = void (*)(void* dst, const void* src);

        // Copy assignment function (assigns dst from src, destroys old dst)
        using copy_assignment_fn = void (*)(void* dst, const void* src);

        destructor_fn       destroy        = nullptr;
        move_constructor_fn move_construct = nullptr;
        move_assignment_fn  move_assign    = nullptr;
        copy_constructor_fn copy_construct = nullptr;
        copy_assignment_fn  copy_assign    = nullptr;

        std::size_t size                  = 0;
        std::size_t alignment             = 0;
        bool        is_trivially_copyable = false;
        bool        is_copy_constructible = false;
        bool        is_move_constructible = false;
    };

    // Type operations factory - generates operations for a specific type
    // Returns a static instance to avoid heap allocations
    template <typename T>
    struct type_operations_factory
    {
        static const type_operations& get() noexcept
        {
            static const type_operations ops = create();
            return ops;
        }

    private:
        static type_operations create() noexcept
        {
            type_operations ops;

            ops.size                  = sizeof(T);
            ops.alignment             = alignof(T);
            ops.is_trivially_copyable = std::is_trivially_copyable_v<T>;
            ops.is_copy_constructible = std::is_copy_constructible_v<T>;
            ops.is_move_constructible = std::is_move_constructible_v<T>;

            // Destructor (always needed for polymorphic types)
            ops.destroy = [](void* obj) noexcept { static_cast<T*>(obj)->~T(); };

            // Move operations
            if constexpr (std::is_move_constructible_v<T>)
            {
                ops.move_construct = [](void* dst, void* src) noexcept
                { new (dst) T(std::move(*static_cast<T*>(src))); };
            }

            if constexpr (std::is_move_assignable_v<T>)
            {
                ops.move_assign = [](void* dst, void* src) noexcept
                { *static_cast<T*>(dst) = std::move(*static_cast<T*>(src)); };
            }

            // Copy operations
            if constexpr (std::is_copy_constructible_v<T>)
            {
                ops.copy_construct = [](void* dst, const void* src)
                { new (dst) T(*static_cast<const T*>(src)); };
            }

            if constexpr (std::is_copy_assignable_v<T>)
            {
                ops.copy_assign = [](void* dst, const void* src)
                { *static_cast<T*>(dst) = *static_cast<const T*>(src); };
            }

            return ops;
        }
    };

    // Convenience function to get type operations without heap allocation
    template <typename T>
    inline const type_operations& get_type_ops() noexcept
    {
        return type_operations_factory<T>::get();
    }

    // Helper to safely move objects using type operations
    inline void safe_move_construct(void* dst, void* src,
                                    const type_operations& ops)
    {
        if (ops.is_trivially_copyable)
        {
            // For trivially copyable types, memcpy is safe and efficient
            std::memcpy(dst, src, ops.size);
        }
        else if (ops.move_construct)
        {
            // Use the type-specific move constructor
            ops.move_construct(dst, src);
        }
        else if (ops.copy_construct)
        {
            // Fall back to copy if move is not available
            ops.copy_construct(dst, src);
        }
        else
        {
            throw std::runtime_error("No move or copy constructor available");
        }
    }

    // Helper to safely copy objects using type operations
    inline void safe_copy_construct(void* dst, const void* src,
                                    const type_operations& ops)
    {
        if (ops.copy_construct)
        {
            ops.copy_construct(dst, src);
        }
        else if (ops.is_trivially_copyable)
        {
            std::memcpy(dst, src, ops.size);
        }
        else
        {
            throw std::runtime_error("Type is not copy constructible");
        }
    }

    // Helper to safely destroy objects
    inline void safe_destroy(void* obj, const type_operations& ops) noexcept
    {
        if (ops.destroy)
        {
            ops.destroy(obj);
        }
    }

    // =============================================================================
    // Type Traits - Utilities for calculating slot size and alignment
    // =============================================================================

    // Type list for registering subclasses
    template <typename... Types>
    struct type_list
    {};

    // Calculate maximum size from a type list
    template <typename TypeList>
    struct max_size;

    template <typename... Types>
    struct max_size<type_list<Types...>>
    {
        static constexpr std::size_t value = std::max({sizeof(Types)...});
    };

    template <typename TypeList>
    inline constexpr std::size_t max_size_v = max_size<TypeList>::value;

    // Calculate maximum alignment from a type list
    template <typename TypeList>
    struct max_alignment;

    template <typename... Types>
    struct max_alignment<type_list<Types...>>
    {
        static constexpr std::size_t value = std::max({alignof(Types)...});
    };

    template <typename TypeList>
    inline constexpr std::size_t max_alignment_v = max_alignment<TypeList>::value;

    // Convenience: slot configuration for a type list
    template <typename TypeList>
    struct slot_config
    {
        static constexpr std::size_t size      = max_size_v<TypeList>;
        static constexpr std::size_t alignment = max_alignment_v<TypeList>;
    };

    // =============================================================================
    // Concepts
    // =============================================================================

    template <typename T>
    concept PolymorphicBase = std::has_virtual_destructor_v<T>;

    template <typename Derived, typename Base, size_t SlotSize, size_t Alignment>
    concept FitsInSlot =
        std::derived_from<Derived, Base> && (sizeof(Derived) <= SlotSize) &&
        (alignof(Derived) <= Alignment);

    // --- Unified Array Container ---
    // Automatically enables copy/move based on contained types

    template <PolymorphicBase Base, size_t N, size_t SlotSize = sizeof(Base),
              size_t Alignment = alignof(Base)>
    class poly_array
    {
    public:
        // Typedefs for STL compatibility
        using value_type             = Base*;
        using size_type              = size_t;
        using difference_type        = std::ptrdiff_t;
        using pointer                = Base**;
        using const_pointer          = Base* const*;
        using reference              = Base*&;
        using const_reference        = Base* const&;
        using iterator               = Base**;
        using const_iterator         = Base* const*;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static_assert(SlotSize >= sizeof(Base), "SlotSize must hold Base");
        static_assert(Alignment >= alignof(Base),
                      "Alignment must be at least alignof(Base)");

    private:
        // Storage for objects and their type information
        struct slot_info
        {
            Base*                  ptr = nullptr;
            const type_operations* ops = nullptr;
        };

        alignas(Alignment) std::byte storage_[N * SlotSize]{};
        std::array<slot_info, N> slots_{};
        bool can_copy_ = false; // Track if all contained types support copying
        bool can_move_ = true;  // Track if all contained types support moving

    public:
        // Default constructor
        poly_array() = default;

        // Copy constructor - enabled only if all contained types are copyable
        // Always available at compile time, runtime check inside, since we don't
        // know whether somebody will store a non-copyable object eventually.
        poly_array(const poly_array& other)
        {
            if (!other.can_copy_)
            {
                throw std::logic_error(
                    "Cannot copy poly_array: contains non-copyable types");
            }
            copy_from(other);
        }

        // Move constructor - always available
        poly_array(poly_array&& other) noexcept
        {
            move_from(std::move(other));
        }

        // Copy assignment
        poly_array& operator=(const poly_array& other)
        {
            if (!other.can_copy_)
            {
                throw std::logic_error(
                    "Cannot copy poly_array: contains non-copyable types");
            }
            if (this != &other)
            {
                clear();
                copy_from(other);
            }
            return *this;
        }

        // Move assignment
        poly_array& operator=(poly_array&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                move_from(std::move(other));
            }
            return *this;
        }

        ~poly_array()
        {
            clear();
        }

        // --- Core Functionality ---

        template <typename Derived, typename... Args>
            requires FitsInSlot<Derived, Base, SlotSize, Alignment> &&
                     std::constructible_from<Derived, Args...>
        Derived* emplace(size_type index, Args&&... args)
        {
            if (index >= N)
            {
                throw std::out_of_range(
                    std::format("poly_array::emplace({}) out of bounds", index));
            }

            // Clean up existing object if present
            if (slots_[index].ptr != nullptr)
            {
                destroy_at(index);
            }

            // Get type operations for this type
            const auto& ops = get_type_ops<Derived>();

            // Construct new object
            void* placement_ptr = get_storage_slot(index);
            auto* new_obj =
                new (placement_ptr) Derived(std::forward<Args>(args)...);

            // Update slot info
            slots_[index] = {.ptr = new_obj, .ops = &ops};

            // Update container capabilities and invalidate iterator cache
            update_capabilities();
            invalidate_cache();

            return new_obj;
        }

        void clear() noexcept
        {
            for (size_type i = 0; i < N; ++i)
            {
                if (slots_[i].ptr != nullptr)
                {
                    destroy_at(i);
                }
            }
            can_copy_ = false;
            can_move_ = true;
            invalidate_cache();
        }

        // --- Element Access ---

        reference at(size_type index)
        {
            if (index >= N)
            {
                throw std::out_of_range(
                    std::format("poly_array::at({}) out of bounds", index));
            }
            return slots_[index].ptr;
        }

        const_reference at(size_type index) const
        {
            if (index >= N)
            {
                throw std::out_of_range(
                    std::format("poly_array::at({}) out of bounds", index));
            }
            return slots_[index].ptr;
        }

        reference operator[](size_type index) noexcept
        {
            assert(index < N);
            return slots_[index].ptr;
        }

        const_reference operator[](size_type index) const noexcept
        {
            assert(index < N);
            return slots_[index].ptr;
        }

        reference front() noexcept
        {
            return slots_[0].ptr;
        }
        const_reference front() const noexcept
        {
            return slots_[0].ptr;
        }
        reference back() noexcept
        {
            return slots_[N - 1].ptr;
        }
        const_reference back() const noexcept
        {
            return slots_[N - 1].ptr;
        }

        // --- Data Access ---

        pointer data() noexcept
        {
            update_ptr_cache();
            return ptr_cache_.data();
        }

        const_pointer data() const noexcept
        {
            update_ptr_cache();
            return ptr_cache_.data();
        }

        // --- Iterators ---

        iterator begin() noexcept
        {
            return data();
        }
        const_iterator begin() const noexcept
        {
            return data();
        }
        const_iterator cbegin() const noexcept
        {
            return begin();
        }

        iterator end() noexcept
        {
            return data() + N;
        }
        const_iterator end() const noexcept
        {
            return data() + N;
        }
        const_iterator cend() const noexcept
        {
            return end();
        }

        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }
        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }
        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }
        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        // --- Capacity ---

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return N == 0;
        }
        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return N;
        }
        [[nodiscard]] constexpr size_type max_size() const noexcept
        {
            return N;
        }

        // --- Query Capabilities ---

        [[nodiscard]] bool is_copyable() const noexcept
        {
            return can_copy_;
        }
        [[nodiscard]] bool is_movable() const noexcept
        {
            return can_move_;
        }

    private:
        // Pointer cache for iterator support (fixed-size array avoids std::vector
        // template issues)
        mutable std::array<Base*, N> ptr_cache_{};
        mutable bool                 cache_valid_ = false;

        void update_ptr_cache() const
        {
            if (!cache_valid_)
            {
                for (size_t i = 0; i < N; ++i)
                {
                    ptr_cache_[i] = slots_[i].ptr;
                }
                cache_valid_ = true;
            }
        }

        void invalidate_cache() const noexcept
        {
            cache_valid_ = false;
        }

        void* get_storage_slot(size_t index) noexcept
        {
            return &storage_[index * SlotSize];
        }

        const void* get_storage_slot(size_t index) const noexcept
        {
            return &storage_[index * SlotSize];
        }

        void destroy_at(size_type index) noexcept
        {
            if (slots_[index].ptr && slots_[index].ops)
            {
                safe_destroy(slots_[index].ptr, *slots_[index].ops);
                slots_[index] = {};
            }
        }

        void copy_from(const poly_array& other)
        {
            for (size_type i = 0; i < N; ++i)
            {
                if (other.slots_[i].ptr && other.slots_[i].ops)
                {
                    void*       dst = get_storage_slot(i);
                    const void* src = other.get_storage_slot(i);

                    safe_copy_construct(dst, src, *other.slots_[i].ops);

                    slots_[i] = {.ptr = static_cast<Base*>(dst),
                                 .ops = other.slots_[i].ops};
                }
            }
            can_copy_ = other.can_copy_;
            can_move_ = other.can_move_;
        }

        void move_from(poly_array&& other) noexcept
        {
            for (size_type i = 0; i < N; ++i)
            {
                if (other.slots_[i].ptr && other.slots_[i].ops)
                {
                    void* dst = get_storage_slot(i);
                    void* src = other.get_storage_slot(i);

                    safe_move_construct(dst, src, *other.slots_[i].ops);

                    slots_[i] = {.ptr = static_cast<Base*>(dst),
                                 .ops = other.slots_[i].ops};

                    // Clean up source
                    other.destroy_at(i);
                }
            }
            can_copy_ = other.can_copy_;
            can_move_ = other.can_move_;
        }

        void update_capabilities()
        {
            bool all_copyable = true;
            bool all_movable  = true;
            bool has_elements = false;

            for (const auto& slot : slots_)
            {
                if (slot.ptr && slot.ops)
                {
                    has_elements = true;
                    if (!slot.ops->is_copy_constructible)
                    {
                        all_copyable = false;
                    }
                    if (!slot.ops->is_move_constructible)
                    {
                        all_movable = false;
                    }
                }
            }

            can_copy_ = !has_elements || all_copyable;
            can_move_ = !has_elements || all_movable;
        }
    };

    // --- Unified Vector Container ---

    template <PolymorphicBase Base, size_t Capacity,
              size_t SlotSize = sizeof(Base), size_t Alignment = alignof(Base)>
    class poly_vector
    {
    public:
        // Typedefs for STL compatibility
        using value_type      = Base*;
        using size_type       = size_t;
        using difference_type = std::ptrdiff_t;
        using pointer         = Base**;
        using const_pointer   = Base* const*;
        using reference       = Base*&;
        using const_reference = Base* const&;

        // Custom iterator that manages pointer array dynamically
        class iterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type        = Base*;
            using difference_type   = std::ptrdiff_t;
            using pointer           = Base**;
            using reference         = Base*&;

            iterator() = default;
            explicit iterator(Base** ptr) : ptr_(ptr) {}

            reference operator*() const
            {
                return *ptr_;
            }
            pointer operator->() const
            {
                return ptr_;
            }

            iterator& operator++()
            {
                ++ptr_;
                return *this;
            }
            iterator operator++(int)
            {
                iterator tmp = *this;
                ++ptr_;
                return tmp;
            }
            iterator& operator--()
            {
                --ptr_;
                return *this;
            }
            iterator operator--(int)
            {
                iterator tmp = *this;
                --ptr_;
                return tmp;
            }

            iterator& operator+=(difference_type n)
            {
                ptr_ += n;
                return *this;
            }
            iterator& operator-=(difference_type n)
            {
                ptr_ -= n;
                return *this;
            }

            iterator operator+(difference_type n) const
            {
                return iterator(ptr_ + n);
            }
            iterator operator-(difference_type n) const
            {
                return iterator(ptr_ - n);
            }

            difference_type operator-(const iterator& other) const
            {
                return ptr_ - other.ptr_;
            }

            reference operator[](difference_type n) const
            {
                return ptr_[n];
            }

            bool operator==(const iterator& other) const
            {
                return ptr_ == other.ptr_;
            }
            bool operator!=(const iterator& other) const
            {
                return ptr_ != other.ptr_;
            }
            bool operator<(const iterator& other) const
            {
                return ptr_ < other.ptr_;
            }
            bool operator<=(const iterator& other) const
            {
                return ptr_ <= other.ptr_;
            }
            bool operator>(const iterator& other) const
            {
                return ptr_ > other.ptr_;
            }
            bool operator>=(const iterator& other) const
            {
                return ptr_ >= other.ptr_;
            }

        private:
            Base** ptr_ = nullptr;
        };

        using const_iterator         = const Base* const*;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static_assert(SlotSize >= sizeof(Base), "SlotSize must hold Base");
        static_assert(Alignment >= alignof(Base),
                      "Alignment must be at least alignof(Base)");

    private:
        // Storage for objects and their type information
        struct slot_info
        {
            Base*                  ptr = nullptr;
            const type_operations* ops = nullptr;
        };

        alignas(Alignment) std::byte storage_[Capacity * SlotSize]{};
        std::array<slot_info, Capacity> slots_{};
        size_t                          size_     = 0;
        bool                            can_copy_ = false;
        bool                            can_move_ = true;

        // Pointer cache for iterator support (fixed-size array avoids std::vector
        // template issues)
        mutable std::array<Base*, Capacity> ptr_cache_{};
        mutable bool                        cache_valid_ = false;

    public:
        // Default constructor
        poly_vector() = default;

        // Copy constructor
        poly_vector(const poly_vector& other)
        {
            if (!other.can_copy_)
            {
                throw std::logic_error(
                    "Cannot copy poly_vector: contains non-copyable types");
            }
            copy_from(other);
        }

        // Move constructor
        poly_vector(poly_vector&& other) noexcept
        {
            move_from(std::move(other));
        }

        // Copy assignment
        poly_vector& operator=(const poly_vector& other)
        {
            if (!other.can_copy_)
            {
                throw std::logic_error(
                    "Cannot copy poly_vector: contains non-copyable types");
            }
            if (this != &other)
            {
                clear();
                copy_from(other);
            }
            return *this;
        }

        // Move assignment
        poly_vector& operator=(poly_vector&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                move_from(std::move(other));
            }
            return *this;
        }

        ~poly_vector()
        {
            clear();
        }

        // --- Core Functionality ---

        template <typename Derived, typename... Args>
            requires FitsInSlot<Derived, Base, SlotSize, Alignment> &&
                     std::constructible_from<Derived, Args...>
        Derived* emplace_back(Args&&... args)
        {
            if (size_ >= Capacity)
            {
                throw std::out_of_range(
                    "poly_vector::emplace_back() - capacity exceeded");
            }

            // Get type operations
            const auto& ops = get_type_ops<Derived>();

            // Construct new object
            void* placement_ptr = get_storage_slot(size_);
            auto* new_obj =
                new (placement_ptr) Derived(std::forward<Args>(args)...);

            // Update slot info
            slots_[size_] = {.ptr = new_obj, .ops = &ops};

            ++size_;
            cache_valid_ = false;
            update_capabilities();

            return new_obj;
        }

        // Push back by copy
        template <typename Derived>
            requires FitsInSlot<Derived, Base, SlotSize, Alignment> &&
                     std::copy_constructible<Derived>
        void push_back(const Derived& value)
        {
            emplace_back<Derived>(value);
        }

        // Push back by move
        template <typename Derived>
            requires FitsInSlot<Derived, Base, SlotSize, Alignment> &&
                     std::move_constructible<Derived>
        void push_back(Derived&& value)
        {
            emplace_back<Derived>(std::forward<Derived>(value));
        }

        template <typename Derived, typename... Args>
            requires FitsInSlot<Derived, Base, SlotSize, Alignment> &&
                     std::constructible_from<Derived, Args...>
        iterator emplace(iterator pos, Args&&... args)
        {
            update_ptr_cache();
            size_t index = static_cast<size_t>(pos - iterator(ptr_cache_.data()));
            if (index > size_)
            {
                throw std::out_of_range(
                    "poly_vector::emplace() - invalid position");
            }

            if (size_ >= Capacity)
            {
                throw std::out_of_range(
                    "poly_vector::emplace() - capacity exceeded");
            }

            // Shift elements to make room (type-safe)
            if (index < size_)
            {
                shift_right(index, 1);
            }

            // Get type operations
            const auto& ops = get_type_ops<Derived>();

            // Construct new element
            void* placement_ptr = get_storage_slot(index);
            auto* new_obj =
                new (placement_ptr) Derived(std::forward<Args>(args)...);

            slots_[index] = {.ptr = new_obj, .ops = &ops};

            ++size_;
            cache_valid_ = false;
            update_capabilities();

            update_ptr_cache();
            return iterator(&ptr_cache_[index]);
        }

        void pop_back()
        {
            if (size_ == 0)
            {
                throw std::out_of_range(
                    "poly_vector::pop_back() - vector is empty");
            }

            --size_;
            destroy_at(size_);
            cache_valid_ = false;
            update_capabilities();
        }

        iterator erase(iterator pos)
        {
            update_ptr_cache();
            if (const size_t index =
                    static_cast<size_t>(pos - iterator(ptr_cache_.data()));
                index >= size_)
            {
                throw std::out_of_range(
                    "poly_vector::erase() - invalid position");
            }

            return erase(pos, pos + 1);
        }

        iterator erase(iterator first, iterator last)
        {
            update_ptr_cache();
            size_t first_index =
                static_cast<size_t>(first - iterator(ptr_cache_.data()));
            size_t last_index =
                static_cast<size_t>(last - iterator(ptr_cache_.data()));

            if (first_index > last_index || last_index > size_)
            {
                throw std::out_of_range("poly_vector::erase() - invalid range");
            }

            size_t count = last_index - first_index;
            if (count == 0)
            {
                update_ptr_cache();
                return iterator(&ptr_cache_[first_index]);
            }

            // Check if we need to shift elements and whether that's possible
            if (last_index < size_ && !can_move_)
            {
                throw std::runtime_error(
                    "poly_vector::erase() - cannot shift elements: contained "
                    "types are neither movable nor copyable. Use pop_back() "
                    "to remove elements from the end.");
            }

            // Destroy elements in range
            for (size_t i = first_index; i < last_index; ++i)
            {
                destroy_at(i);
            }

            // Shift remaining elements left (type-safe)
            if (last_index < size_)
            {
                shift_left(last_index, count);
            }

            size_        -= count;
            cache_valid_  = false;
            update_capabilities();

            update_ptr_cache();
            return iterator(&ptr_cache_[first_index]);
        }

        void clear() noexcept
        {
            for (size_t i = 0; i < size_; ++i)
            {
                destroy_at(i);
            }
            size_        = 0;
            can_copy_    = false;
            can_move_    = true;
            cache_valid_ = false;
        }

        void resize(size_type new_size)
        {
            if (new_size > Capacity)
            {
                throw std::out_of_range(
                    "poly_vector::resize() - exceeds capacity");
            }

            while (size_ > new_size)
            {
                pop_back();
            }

            while (size_ < new_size)
            {
                slots_[size_].ptr = nullptr;
                ++size_;
            }
            cache_valid_ = false;
        }

        // --- Element Access ---

        reference operator[](size_type index)
        {
            assert(index < size_);
            return slots_[index].ptr;
        }

        const_reference operator[](size_type index) const
        {
            assert(index < size_);
            return slots_[index].ptr;
        }

        reference at(size_type index)
        {
            if (index >= size_)
            {
                throw std::out_of_range(
                    "poly_vector::at() - index out of bounds");
            }
            return slots_[index].ptr;
        }

        const_reference at(size_type index) const
        {
            if (index >= size_)
            {
                throw std::out_of_range(
                    "poly_vector::at() - index out of bounds");
            }
            return slots_[index].ptr;
        }

        reference front()
        {
            if (size_ == 0)
            {
                throw std::out_of_range("poly_vector::front() - vector is empty");
            }
            return slots_[0].ptr;
        }

        const_reference front() const
        {
            if (size_ == 0)
            {
                throw std::out_of_range("poly_vector::front() - vector is empty");
            }
            return slots_[0].ptr;
        }

        reference back()
        {
            if (size_ == 0)
            {
                throw std::out_of_range("poly_vector::back() - vector is empty");
            }
            return slots_[size_ - 1].ptr;
        }

        const_reference back() const
        {
            if (size_ == 0)
            {
                throw std::out_of_range("poly_vector::back() - vector is empty");
            }
            return slots_[size_ - 1].ptr;
        }

        // --- Iterators ---

        iterator begin()
        {
            update_ptr_cache();
            return iterator(ptr_cache_.data());
        }

        const_iterator begin() const
        {
            update_ptr_cache();
            return ptr_cache_.data();
        }

        const_iterator cbegin() const
        {
            return begin();
        }

        iterator end()
        {
            update_ptr_cache();
            return iterator(ptr_cache_.data() + size_);
        }

        const_iterator end() const
        {
            update_ptr_cache();
            return ptr_cache_.data() + size_;
        }

        const_iterator cend() const
        {
            return end();
        }

        reverse_iterator rbegin()
        {
            return reverse_iterator(end());
        }
        const_reverse_iterator rbegin() const
        {
            return const_reverse_iterator(end());
        }
        reverse_iterator rend()
        {
            return reverse_iterator(begin());
        }
        const_reverse_iterator rend() const
        {
            return const_reverse_iterator(begin());
        }

        // --- Data Access ---

        pointer data() noexcept
        {
            update_ptr_cache();
            return ptr_cache_.data();
        }

        const_pointer data() const noexcept
        {
            update_ptr_cache();
            return ptr_cache_.data();
        }

        // --- Capacity ---

        [[nodiscard]] bool empty() const noexcept
        {
            return size_ == 0;
        }
        [[nodiscard]] size_type size() const noexcept
        {
            return size_;
        }
        [[nodiscard]] constexpr size_type max_size() const noexcept
        {
            return Capacity;
        }
        [[nodiscard]] constexpr size_type capacity() const noexcept
        {
            return Capacity;
        }

        void reserve(size_type new_cap)
        {
            if (new_cap > Capacity)
            {
                throw std::length_error(
                    "poly_vector::reserve() - exceeds fixed capacity");
            }
            // No-op for fixed capacity container
        }

        void shrink_to_fit() noexcept
        {
            // No-op for fixed capacity container
        }

        // --- Query Capabilities ---

        [[nodiscard]] bool is_copyable() const noexcept
        {
            return can_copy_;
        }
        [[nodiscard]] bool is_movable() const noexcept
        {
            return can_move_;
        }

    private:
        void* get_storage_slot(size_t index) noexcept
        {
            return &storage_[index * SlotSize];
        }

        const void* get_storage_slot(size_t index) const noexcept
        {
            return &storage_[index * SlotSize];
        }

        void destroy_at(size_type index) noexcept
        {
            if (slots_[index].ptr && slots_[index].ops)
            {
                safe_destroy(slots_[index].ptr, *slots_[index].ops);
                slots_[index] = {};
            }
        }

        // Type-safe shift operations that properly move objects
        void shift_right(size_t start_index, size_t count)
        {
            // Move objects from end to start
            for (size_t i = size_; i > start_index; --i)
            {
                size_t src = i - 1;
                size_t dst = src + count;

                if (slots_[src].ptr && slots_[src].ops)
                {
                    void* dst_storage = get_storage_slot(dst);
                    void* src_storage = get_storage_slot(src);

                    // Move construct at new location
                    safe_move_construct(dst_storage, src_storage,
                                        *slots_[src].ops);

                    // Update slot info
                    slots_[dst]     = slots_[src];
                    slots_[dst].ptr = static_cast<Base*>(dst_storage);

                    // Destroy at old location
                    safe_destroy(src_storage, *slots_[src].ops);
                    slots_[src] = {};
                }
            }
        }

        void shift_left(size_t start_index, size_t count)
        {
            // Move objects from start to end
            for (size_t i = start_index; i < size_; ++i)
            {
                size_t dst = i - count;

                if (slots_[i].ptr && slots_[i].ops)
                {
                    void* dst_storage = get_storage_slot(dst);
                    void* src_storage = get_storage_slot(i);

                    // Move construct at new location
                    safe_move_construct(dst_storage, src_storage, *slots_[i].ops);

                    // Update slot info
                    slots_[dst]     = slots_[i];
                    slots_[dst].ptr = static_cast<Base*>(dst_storage);

                    // Destroy at old location
                    safe_destroy(src_storage, *slots_[i].ops);
                    slots_[i] = {};
                }
            }
        }

        void copy_from(const poly_vector& other)
        {
            size_ = other.size_;
            for (size_type i = 0; i < size_; ++i)
            {
                if (other.slots_[i].ptr && other.slots_[i].ops)
                {
                    void*       dst = get_storage_slot(i);
                    const void* src = other.get_storage_slot(i);

                    safe_copy_construct(dst, src, *other.slots_[i].ops);

                    slots_[i] = {.ptr = static_cast<Base*>(dst),
                                 .ops = other.slots_[i].ops};
                }
            }
            can_copy_    = other.can_copy_;
            can_move_    = other.can_move_;
            cache_valid_ = false;
        }

        void move_from(poly_vector&& other) noexcept
        {
            size_ = other.size_;
            for (size_type i = 0; i < size_; ++i)
            {
                if (other.slots_[i].ptr && other.slots_[i].ops)
                {
                    void* dst = get_storage_slot(i);
                    void* src = other.get_storage_slot(i);

                    safe_move_construct(dst, src, *other.slots_[i].ops);

                    slots_[i] = {.ptr = static_cast<Base*>(dst),
                                 .ops = other.slots_[i].ops};

                    // Clean up source
                    other.destroy_at(i);
                }
            }
            can_copy_    = other.can_copy_;
            can_move_    = other.can_move_;
            other.size_  = 0;
            cache_valid_ = false;
        }

        void update_capabilities()
        {
            bool all_copyable = true;
            bool all_movable  = true;
            bool has_elements = false;

            for (size_t i = 0; i < size_; ++i)
            {
                if (slots_[i].ptr && slots_[i].ops)
                {
                    has_elements = true;
                    if (!slots_[i].ops->is_copy_constructible)
                    {
                        all_copyable = false;
                    }
                    if (!slots_[i].ops->is_move_constructible)
                    {
                        all_movable = false;
                    }
                }
            }

            can_copy_ = !has_elements || all_copyable;
            can_move_ = !has_elements || all_movable;
        }

        void update_ptr_cache() const
        {
            if (!cache_valid_)
            {
                for (size_t i = 0; i < size_; ++i)
                {
                    ptr_cache_[i] = slots_[i].ptr;
                }
                // Fill rest with nullptr for safety
                for (size_t i = size_; i < Capacity; ++i)
                {
                    ptr_cache_[i] = nullptr;
                }
                cache_valid_ = true;
            }
        }
    };

    // Type aliases for cleaner API
    template <PolymorphicBase Base, size_t N, size_t SlotSize = sizeof(Base),
              size_t Alignment = alignof(Base)>
    using array = poly_array<Base, N, SlotSize, Alignment>;

    template <PolymorphicBase Base, size_t Capacity,
              size_t SlotSize = sizeof(Base), size_t Alignment = alignof(Base)>
    using vector = poly_vector<Base, Capacity, SlotSize, Alignment>;

} // namespace inline_poly

#endif // INLINE_POLY_H
