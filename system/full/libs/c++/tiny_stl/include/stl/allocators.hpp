// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>

#include "utility.hpp"

namespace tiny_stl {

namespace details {

template <typename T>
inline T* allocateHelper(std::ptrdiff_t size, T* /* p */) {
    std::set_new_handler(nullptr);
    T* tmp = reinterpret_cast<T*>(
        ::operator new(static_cast<std::size_t>(size * sizeof(T))));
    if (tmp == nullptr) {
        printf("out of memery");
        exit(EXIT_FAILURE);
    }
    return tmp;
}

template <typename T>
inline void deallocateHelper(T* buffer) noexcept {
    ::operator delete(buffer);
}

template <typename T, typename... Args>
inline void constructHelper(T* p, Args&&... args) {
    new (const_cast<void*>(static_cast<const volatile void*>(p)))
        T(tiny_stl::forward<Args>(args)...); // placement new, ctor T
}

} // namespace details

template <typename T>
inline void destroy_at(T* ptr) {
    ptr->~T();
}

template <typename T>
class allocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = tiny_stl::true_type; // c++14
    using is_always_equal = tiny_stl::true_type;                        // c++17

    using NotUserSpecialized = void;

public:
    allocator() noexcept {
    } // do nothing
    allocator(const allocator<T>&) noexcept {
    }

    template <typename Other>
    allocator(const allocator<Other>&) noexcept {
    }

    template <typename Other>
    allocator<T>& operator=(const allocator<Other>&) noexcept {
        // do nothing
        return *this;
    }

    template <typename U>
    struct rebind {
        using other = allocator<U>;
    };

    pointer allocate(size_type n) {
        return details::allocateHelper(static_cast<difference_type>(n),
                                       reinterpret_cast<pointer>(0));
    }

    void deallocate(pointer p, std::size_t n) noexcept {
        details::deallocateHelper(p);
    }

    pointer address(reference x) const noexcept {
        return tiny_stl::addressof(x);
    }

    const_pointer address(const_reference x) const noexcept {
        return reinterpret_cast<const_pointer>(&x);
    }

    template <typename Obj, typename... Args>
    void construct(Obj* p, Args&&... args) {
        details::constructHelper(p, tiny_stl::forward<Args>(args)...);
    }

    template <typename Obj>
    void destroy(Obj* ptr) {
        tiny_stl::destroy_at(ptr);
    }

    size_type max_size() const noexcept {
        return (std::numeric_limits<std::size_t>::max() / sizeof(T));
    }
}; // class allocator<T>

template <>
class allocator<void> {
public:
    using value_type = void;
    using pointer = void*;
    using const_pointer = const void*;

    template <class U>
    struct rebind {
        using other = allocator<U>;
    };

public:
    allocator() noexcept {
    }
    allocator(const allocator<void>&) noexcept {
    }

    template <typename Other>
    allocator(const allocator<Other>&) noexcept {
    }

    template <typename Other>
    allocator<void>& operator=(const allocator<Other>&) noexcept {
        return *this;
    }
}; // class allocator<void>

template <typename T>
inline bool operator==(const allocator<T>& lhs,
                       const allocator<T>& rhs) noexcept {
    return true;
}

template <typename T>
inline bool operator!=(const allocator<T>& lhs,
                       const allocator<T>& rhs) noexcept {
    return (!(lhs == rhs));
}

template <typename Alloc>
inline void swapAllocHelper(Alloc& lhs, Alloc& rhs, true_type) {
    swapADL(lhs, rhs);
}

template <typename Alloc>
inline void swapAllocHelper(Alloc& lhs, Alloc& rhs, false_type) {
    assert(lhs == rhs);
}

template <typename Alloc>
struct allocator_traits;

template <typename Alloc>
inline void swapAlloc(Alloc& lhs, Alloc& rhs) noexcept {
    typename allocator_traits<Alloc>::propagate_on_container_swap tag;
    swapAllocHelper(lhs, rhs, tag);
}

} // namespace tiny_stl
