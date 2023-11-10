// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "algorithm.hpp"
#include "iterator.hpp"
#include "memory.hpp"

namespace tiny_stl {

template <typename T, std::size_t Size>
class ArrayConstIterator {
public:
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

public:
#ifndef NDEBUG // DEBUG
    pointer ptr;
    std::size_t idx;

    constexpr ArrayConstIterator() : ptr(nullptr), idx(0) {
    }

    constexpr explicit ArrayConstIterator(pointer p, std::size_t offset = 0)
        : ptr(p), idx(offset) {
    }

    constexpr reference operator*() const {
        assert(ptr != nullptr && idx < Size);
        return ptr[idx];
    }

    constexpr pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    constexpr ArrayConstIterator& operator++() {
        assert(ptr != nullptr && idx < Size);
        ++idx;
        return *this;
    }

    constexpr ArrayConstIterator operator++(int) {
        ArrayConstIterator tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr ArrayConstIterator& operator--() {
        assert(ptr != nullptr && idx != 0);
        --idx;
        return *this;
    }

    constexpr ArrayConstIterator operator--(int) {
        ArrayConstIterator tmp = *this;
        --*this;
        return tmp;
    }

    constexpr ArrayConstIterator& operator+=(std::ptrdiff_t offset) {
        assert(idx + offset <= Size);
        idx += offset;
        return *this;
    }

    constexpr ArrayConstIterator operator+(std::ptrdiff_t offset) const {
        ArrayConstIterator tmp = *this;
        return tmp += offset;
    }

    constexpr ArrayConstIterator& operator-=(std::ptrdiff_t offset) {
        return (*this += (-offset));
    }

    constexpr ArrayConstIterator operator-(std::ptrdiff_t offset) const {
        ArrayConstIterator tmp = *this;
        return tmp -= offset;
    }

    constexpr std::ptrdiff_t operator-(const ArrayConstIterator& rhs) const {
        assert(ptr == rhs.ptr);
        return static_cast<std::ptrdiff_t>(idx - rhs.idx);
    }

    constexpr reference operator[](std::ptrdiff_t offset) const {
        return *(*this + offset);
    }

    constexpr bool operator==(const ArrayConstIterator& rhs) const {
        assert(ptr == rhs.ptr);
        return idx == rhs.idx;
    }

    constexpr bool operator!=(const ArrayConstIterator& rhs) const {
        return (!(*this == rhs));
    }

    constexpr bool operator<(const ArrayConstIterator& rhs) const {
        assert(ptr != rhs.ptr);
        return idx < rhs.idx;
    }

    constexpr bool operator<=(const ArrayConstIterator& rhs) const {
        return (!(rhs < *this));
    }

    constexpr bool operator>(const ArrayConstIterator& rhs) const {
        return rhs < *this;
    }

    constexpr bool operator>=(const ArrayConstIterator& rhs) const {
        return !(*this < rhs);
    }
#else
    pointer ptr;

    constexpr ArrayConstIterator() : ptr(nullptr) {
    }

    constexpr explicit ArrayConstIterator(pointer p, std::size_t offset = 0)
        : ptr(p + offset) {
    }

    constexpr reference operator*() const {
        return *ptr;
    }

    constexpr pointer operator->() const {
        return ptr;
    }

    constexpr ArrayConstIterator& operator++() {
        ++ptr;
        return *this;
    }

    constexpr ArrayConstIterator operator++(int) {
        ArrayConstIterator tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr ArrayConstIterator& operator--() {
        --ptr;
        return *this;
    }

    constexpr ArrayConstIterator operator--(int) {
        ArrayConstIterator tmp = *this;
        --*this;
        return tmp;
    }

    constexpr ArrayConstIterator& operator+=(std::ptrdiff_t offset) {
        ptr += offset;
        return *this;
    }

    constexpr ArrayConstIterator operator+(std::ptrdiff_t offset) const {
        ArrayConstIterator tmp = *this;
        return tmp += offset;
    }

    constexpr ArrayConstIterator& operator-=(std::ptrdiff_t offset) {
        return (*this += (-offset));
    }

    constexpr ArrayConstIterator operator-(std::ptrdiff_t offset) const {
        ArrayConstIterator tmp = *this;
        return tmp -= offset;
    }

    constexpr std::ptrdiff_t operator-(const ArrayConstIterator& rhs) const {
        return ptr - rhs.ptr;
    }

    constexpr reference operator[](std::ptrdiff_t offset) const {
        return *(*this + offset);
    }

    constexpr bool operator==(const ArrayConstIterator& rhs) const {
        return ptr == rhs.ptr;
    }

    constexpr bool operator!=(const ArrayConstIterator& rhs) const {
        return (!(*this == rhs));
    }

    constexpr bool operator<(const ArrayConstIterator& rhs) const {
        return ptr < rhs.ptr;
    }

    constexpr bool operator<=(const ArrayConstIterator& rhs) const {
        return (!(rhs < *this));
    }

    constexpr bool operator>(const ArrayConstIterator& rhs) const {
        return rhs < *this;
    }

    constexpr bool operator>=(const ArrayConstIterator& rhs) const {
        return !(*this < rhs);
    }

#endif // !NDEBUG
};     // class ArrayConstIterator<T>

template <typename T, std::size_t Size>
constexpr ArrayConstIterator<T, Size>
operator+(std::ptrdiff_t offset, ArrayConstIterator<T, Size> rhs) {
    return rhs += offset;
}

template <typename T, std::size_t Size>
class ArrayIterator : public ArrayConstIterator<T, Size> {
private:
    using Base = ArrayConstIterator<T, Size>;

public:
    using pointer = T*;
    using reference = T&;

    constexpr ArrayIterator() {
    }

    constexpr explicit ArrayIterator(pointer p, std::ptrdiff_t offset)
        : Base(p, offset) {
    }

    constexpr reference operator*() const {
        return const_cast<reference>(Base::operator*());
    }

    constexpr pointer operator->() const {
        return const_cast<pointer>(Base::operator->());
    }

    constexpr ArrayIterator& operator++() {
        ++*static_cast<Base*>(this);
        return *this;
    }

    constexpr ArrayIterator operator++(int) {
        ArrayIterator tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr ArrayIterator& operator--() {
        --*static_cast<Base*>(this);
        return *this;
    }

    constexpr ArrayIterator operator--(int) {
        ArrayIterator tmp = *this;
        --*this;
        return tmp;
    }

    constexpr ArrayIterator& operator+=(std::ptrdiff_t offset) {
        *static_cast<Base*>(this) += offset;
        return *this;
    }

    constexpr ArrayIterator operator+(std::ptrdiff_t offset) const {
        ArrayIterator tmp = *this;
        return tmp += offset;
    }

    constexpr ArrayIterator& operator-=(std::ptrdiff_t offset) {
        return (*this += (-offset));
    }

    constexpr ArrayIterator operator-(std::ptrdiff_t offset) const {
        ArrayIterator tmp = *this;
        return tmp -= offset;
    }

    constexpr std::ptrdiff_t operator-(const Base& rhs) const {
        return *(static_cast<const Base*>(this)) - rhs;
    }

    constexpr reference operator[](std::ptrdiff_t offset) const {
        return *(*this + offset);
    }
}; // class ArrayIterator

template <typename T, std::size_t Size>
constexpr ArrayIterator<T, Size> operator+(std::ptrdiff_t offset,
                                           ArrayIterator<T, Size> rhs) {
    return rhs += offset;
}

template <typename T, std::size_t Size>
class array {
public:
    T elements[Size];

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = ArrayIterator<T, Size>;
    using const_iterator = ArrayConstIterator<T, Size>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

    // Constructor implicit declaration
    // Destructor  implicit declaration
    // operator=() implicit declaration

    void assign(const T& value) {
        tiny_stl::fill_n(elements, Size, value);
    }

    void fill(const T& value) {
        tiny_stl::fill_n(elements, Size, value);
    }

    reference at(size_type pos) {
        assert(pos < Size);
        return elements[pos];
    }

    constexpr const_reference at(size_type pos) const {
        assert(pos < Size);
        return elements[pos];
    }

    reference operator[](size_type pos) {
        return elements[pos];
    }

    constexpr const_reference operator[](size_type pos) const {
        return elements[pos];
    }

    reference front() {
        assert(!empty());
        return elements[0];
    }

    constexpr const_reference front() const {
        assert(!empty());
        return elements[0];
    }

    reference back() {
        assert(!empty());
        return elements[Size - 1];
    }

    constexpr const_reference back() const {
        assert(!empty());
        return elements[Size - 1];
    }

    pointer data() noexcept {
        return elements;
    }

    constexpr const pointer data() const noexcept {
        return elements;
    }

    constexpr iterator begin() noexcept {
        return iterator(elements, 0);
    }

    constexpr const_iterator begin() const noexcept {
        return const_iterator(elements, 0);
    }

    constexpr const_iterator cbegin() const noexcept {
        return const_iterator(elements, 0);
    }

    constexpr iterator end() noexcept {
        return iterator(elements, Size);
    }

    constexpr const_iterator end() const noexcept {
        return const_iterator(elements, Size);
    }

    constexpr const_iterator cend() const noexcept {
        return const_iterator(elements, Size);
    }

    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    constexpr const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(begin());
    }

    constexpr bool empty() const noexcept {
        return false;
    }

    constexpr size_type size() const noexcept {
        return Size;
    }

    constexpr size_type max_size() const noexcept {
        return Size;
    }

    // O(n)
    void swap(array<T, Size>& other) noexcept(is_nothrow_swappable<T>::value) {
        swap_ranges(elements, elements + Size, other.elements);
    }
}; // array<T, Size>

#define ARRAY0_OUT_OF_RANGE false

template <typename T>
class array<T, 0> {
public:
    T element[1];

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

    void assign(const T&) {
    }

    void fill(const T&) {
    }

    reference at(size_type) {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    const_reference at(size_type) const {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    reference operator[](size_type) {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    const_reference operator[](size_type) const {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    reference front() {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    const_reference front() const {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    reference back() {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    const_reference back() const {
        assert(ARRAY0_OUT_OF_RANGE);
        return element[0];
    }

    void swap(array&) noexcept {
    }

    constexpr iterator begin() {
        return iterator(0);
    }

    constexpr const_iterator begin() const {
        return iterator(0);
    }

    constexpr iterator end() {
        return iterator(0);
    }

    constexpr const_iterator end() const {
        return iterator(0);
    }

    constexpr reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    constexpr const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    constexpr reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    constexpr const_reverse_iterator rend() const {
        return constreverse_iterator(begin());
    }

    constexpr const_iterator cbegin() const {
        return iterator(0);
    }

    constexpr const_iterator cend() const {
        return iterator(0);
    }

    constexpr const_reverse_iterator crbegin() const {
        return rbegin();
    }

    constexpr const_reverse_iterator crend() const {
        return rend();
    }

    constexpr size_type size() const {
        return 0;
    }

    constexpr size_type max_size() const {
        return 0;
    }

    constexpr bool empty() const {
        return true;
    }

    constexpr T* data() {
        return nullptr;
    }

    constexpr const T* data() const {
        return nullptr;
    }
}; // array<T, 0>

// array non-member function
template <typename T, std::size_t Size>
bool operator==(const array<T, Size>& lhs, const array<T, Size>& rhs) {
    return tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, std::size_t Size>
bool operator!=(const array<T, Size>& lhs, const array<T, Size>& rhs) {
    return (!(lhs == rhs));
}

template <typename T, std::size_t Size>
bool operator<(const array<T, Size>& lhs, const array<T, Size>& rhs) {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename T, std::size_t Size>
bool operator>(const array<T, Size>& lhs, const array<T, Size>& rhs) {
    return rhs < lhs;
}

template <typename T, std::size_t Size>
bool operator<=(const array<T, Size>& lhs, const array<T, Size>& rhs) {
    return !(rhs < lhs);
}

template <typename T, std::size_t Size>
bool operator>=(const array<T, Size>& lhs, const array<T, Size>& rhs) {
    return !(lhs < rhs);
}

template <std::size_t Idx, typename T, std::size_t Size>
constexpr T& get(array<T, Size>& arr) noexcept {
    static_assert(Idx < Size, "array index out of ranges");
    return arr.elements[Idx];
}

template <std::size_t Idx, typename T, std::size_t Size>
constexpr T&& get(array<T, Size>&& arr) noexcept {
    static_assert(Idx < Size, "array index out of ranges");
    return tiny_stl::move(arr.elements[Idx]);
}

template <std::size_t Idx, typename T, std::size_t Size>
constexpr const T& get(const array<T, Size>& arr) noexcept {
    static_assert(Idx < Size, "array index out of ranges");
    return arr.elements[Idx];
}

template <std::size_t Idx, typename T, std::size_t Size>
constexpr const T&& get(const array<T, Size>&& arr) noexcept {
    static_assert(Idx < Size, "array index out of ranges");
    return tiny_stl::move(arr.elements[Idx]);
}

template <typename T, std::size_t Size,
          typename = enable_if_t<Size == 0 || is_swappable<T>::value>>
void swap(array<T, Size>& lhs,
          array<T, Size>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    return lhs.swap(rhs);
}

// tuple_size to array
template <typename T, std::size_t Size>
struct tuple_size<array<T, Size>> : integral_constant<std::size_t, Size> {};

// tuple_element to array
template <std::size_t Idx, typename T, std::size_t Size>
struct tuple_element<Idx, array<T, Size>> {
    static_assert(Idx < Size, "array index out of range");
    using type = T;
};

} // namespace tiny_stl
