// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "string.hpp"

namespace tiny_stl {

template <typename CharT>
struct StringViewIterator {
    using iterator_category = random_access_iterator_tag;
    using value_type = CharT;
    using pointer = const CharT*;
    using reference = const CharT&;
    using difference_type = std::ptrdiff_t;
    using Self = StringViewIterator<CharT>;

#ifndef NDEBUG // DEBUG
    constexpr StringViewIterator() noexcept : ptr(nullptr), size(0), index(0) {
    }

    constexpr StringViewIterator(const pointer p, const std::size_t s,
                                 const std::size_t idx) noexcept
        : ptr(p), size(s), index(idx) {
    }

    constexpr reference operator*() const noexcept {
        assert(ptr != nullptr);
        assert(index < size);
        return ptr[index];
    }

    constexpr pointer operator->() const noexcept {
        assert(ptr != nullptr);
        assert(index < size);
        return ptr;
    }

    constexpr Self& operator++() noexcept {
        assert(ptr != nullptr);
        assert(index < size);
        ++index;
        return *this;
    }

    constexpr Self operator++(int) noexcept {
        Self tmp{*this};
        ++*this;
        return tmp;
    }

    constexpr Self& operator--() noexcept {
        assert(ptr != nullptr);
        assert(index != 0);
        --index;
        return *this;
    }

    constexpr Self operator--(int) noexcept {
        Self tmp{*this};
        --*this;
        return tmp;
    }

    constexpr Self& operator+=(const difference_type off) noexcept {
        assert(ptr != nullptr);
        if (off < 0) {
            assert(index >= static_cast<std::size_t>(-off));
        } else if (off > 0) {
            assert(index + off <= size);
        }

        index += off;
        return *this;
    }

    constexpr Self operator+(const difference_type off) const noexcept {
        Self tmp{*this};
        tmp += off;
        return tmp;
    }

    constexpr Self operator-=(const difference_type off) noexcept {
        assert(ptr != nullptr);
        if (off > 0) {
            assert(index >= static_cast<std::size_t>(off));
        } else if (off < 0) {
            assert(index + static_cast<std::size_t>(-off) <= size);
        }

        index -= off;
        return *this;
    }

    constexpr Self operator-(const difference_type off) const noexcept {
        Self tmp{*this};
        tmp -= off;
        return tmp;
    }

    constexpr difference_type operator-(const Self& rhs) const noexcept {
        assert(ptr == rhs.ptr && size == rhs.size);
        return index - rhs.index;
    }

    constexpr reference operator[](const difference_type off) const noexcept {
        return *(*this + off);
    }

    constexpr bool operator==(const Self& rhs) const noexcept {
        assert(ptr == rhs.ptr && size == rhs.size);
        return index == rhs.index;
    }

    constexpr bool operator<(const Self& rhs) const noexcept {
        assert(ptr == rhs.ptr && size == rhs.size);
        return index < rhs.index;
    }

    pointer ptr;
    std::size_t size;
    std::size_t index;

#else
    constexpr StringViewIterator() : ptr(nullptr) {
    }
    constexpr StringViewIterator(const CharT* p) : ptr(p) {
    }

    constexpr reference operator*() const noexcept {
        return *ptr;
    }

    constexpr pointer operator->() const noexcept {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    constexpr Self& operator++() noexcept {
        ++ptr;
        return *this;
    }

    constexpr Self operator++(int) const noexcept {
        Self tmp{*this};
        ++*this;
        return tmp;
    }

    constexpr Self& operator--() noexcept {
        --ptr;
        return *this;
    }

    constexpr Self operator--(int) const noexcept {
        Self tmp{*this};
        --*this;
        return tmp;
    }

    constexpr Self& operator+=(const difference_type off) noexcept {
        ptr += off;
        return *this;
    }

    constexpr Self operator+(const difference_type off) const noexcept {
        Self tmp{*this};
        tmp += off;
        return tmp;
    }

    constexpr Self& operator-=(const difference_type off) noexcept {
        ptr -= off;
        return *this;
    }

    constexpr Self operator-(const difference_type off) const noexcept {
        Self tmp{*this};
        tmp -= off;
        return tmp;
    }

    constexpr difference_type operator-(const Self& rhs) const noexcept {
        return ptr - rhs.ptr;
    }

    constexpr reference operator[](const difference_type off) const noexcept {
        return *(*this + off);
    }

    constexpr bool operator==(const Self& rhs) const noexcept {
        return ptr == rhs.ptr;
    }

    constexpr bool operator<(const Self& rhs) const noexcept {
        return ptr < rhs.ptr;
    }

    pointer ptr;
#endif // !NDEBUG

    constexpr bool operator!=(const Self& rhs) const noexcept {
        return !(*this == rhs);
    }

    constexpr bool operator>(const Self& rhs) const noexcept {
        return rhs < *this;
    }

    constexpr bool operator<=(const Self& rhs) const noexcept {
        return !(rhs < *this);
    }

    constexpr bool operator>=(const Self& rhs) const noexcept {
        return !(*this < rhs);
    }
};

template <typename CharT>
constexpr StringViewIterator<CharT>
operator+(const typename StringViewIterator<CharT>::difference_type off,
          StringViewIterator<CharT> rhs) noexcept {
    rhs += off;
    return rhs;
}

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_string_view {
public:
    using value_type = CharT;
    using traits_type = Traits;
    using pointer = CharT*;
    using const_pointer = const CharT*;
    using reference = CharT&;
    using const_reference = const CharT&;
    using const_iterator = StringViewIterator<CharT>;
    using iterator = const_iterator;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

public:
    static constexpr size_type npos = static_cast<size_type>(-1);

    constexpr basic_string_view() noexcept : mData(nullptr), mSize(0) {
    }

    constexpr basic_string_view(const basic_string_view& rhs) noexcept =
        default;

    constexpr basic_string_view(const CharT* s, size_type count)
        : mData(s), mSize(count) {
    }

    constexpr basic_string_view&
    operator=(const basic_string_view& rhs) noexcept = default;

    // use constexpr version instead of char_traits::length
    // std::char_traits constexpr length need C++17
    // C++14 should be able to implement it
    // TODO char_traits
    XCONSTEXPR14 basic_string_view(const CharT* s)
        : mData(s), mSize(getStringLength(s)) {
    }

    constexpr const_iterator begin() const noexcept {
#ifndef NDEBUG // DEBUG
        return const_iterator{mData, mSize, 0};
#else
        return const_iterator{mData};
#endif // !NDEBUG
    }

    constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    constexpr const_iterator end() const noexcept {
#ifndef NDEBUG
        return const_iterator{mData, mSize, mSize};
#else
        return const_iterator{mData + mSize};
#endif // !NDEBUG
    }

    constexpr const_iterator cend() const noexcept {
        return end();
    }

    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator{end()};
    }

    constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator{begin()};
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }

    XCONSTEXPR14 const_reference operator[](size_type pos) const {
        assert(pos < mSize);
        return mData[pos];
    }

    XCONSTEXPR14 const_reference at(size_type pos) const {
        checkPostionEqual(pos);

        return mData[pos];
    }

    constexpr const_reference front() const {
        return operator[](0);
    }

    constexpr const_reference back() const {
        return operator[](mSize - 1);
    }

    constexpr const_pointer data() const noexcept {
        return mData;
    }

    constexpr size_type size() const noexcept {
        return mSize;
    }

    constexpr size_type length() const noexcept {
        return mSize;
    }

    constexpr size_type max_size() const noexcept {
        return static_cast<size_type>(-1) / sizeof(CharT);
    }

    constexpr bool empty() const noexcept {
        return mSize == 0;
    }

    XCONSTEXPR14 void remove_prefix(size_type n) {
        assert(n <= mSize);
        mData += n;
        mSize -= n;
    }

    XCONSTEXPR14 void remove_suffix(size_type n) {
        assert(n < mSize);
        mSize -= n;
    }

    XCONSTEXPR14 void swap(basic_string_view& rhs) noexcept {
        if (this != &rhs) {
            basic_string_view tmp{rhs};
            rhs = *this;
            *this = tmp;
        }
    }

    size_type copy(CharT* dst, size_type count, size_type pos = 0) const {
        checkPosition(pos);
        count = min(count, mSize - pos);
        Traits::copy(dst, mData + pos, count);
        return count;
    }

    XCONSTEXPR14 basic_string_view substr(size_type pos = 0,
                                          size_type count = npos) const {
        checkPosition(pos);
        count = min(count, mSize - pos);
        return basic_string_view{mData + pos, count};
    }

    XCONSTEXPR14 int compare(basic_string_view rhs) const noexcept {
        return compareAux(mData, mSize, rhs.mData, rhs.mSize);
    }

    XCONSTEXPR14 int compare(size_type pos1, size_type count1,
                             basic_string_view rhs) const {
        return substr(pos1, count1).compare(rhs);
    }

    XCONSTEXPR14 int compare(size_type pos1, size_type count1,
                             basic_string_view rhs, size_type pos2,
                             size_type count2) const {
        return substr(pos1, count1).compare(rhs.substr(pos2, count2));
    }

    XCONSTEXPR14 int compare(const CharT* str) const {
        return compare(basic_string_view{str});
    }

    XCONSTEXPR14
    int compare(size_type pos1, size_type count1, const CharT* str) const {
        return substr(pos1, count1).compare(str);
    }

    XCONSTEXPR14
    int compare(size_type pos1, size_type count1, const CharT* str,
                size_type count2) const {
        return substr(pos1, count1).compare(basic_string_view{str, count2});
    }

    XCONSTEXPR14 bool starts_with(basic_string_view rhs) const noexcept {
        return mSize >= rhs.mSize && 
            Traits::compare(this->mData, rhs.mData, rhs.mSize) == 0;
    }

    XCONSTEXPR14 bool starts_with(CharT ch) const noexcept {
        return starts_with(basic_string_view{&ch, 1});
    }

    XCONSTEXPR14 bool starts_with(const CharT* str) const noexcept {
        return starts_with(basic_string_view{str});
    }

    XCONSTEXPR14 bool ends_with(basic_string_view rhs) const noexcept {
        const size_type rightSize = rhs.mSize;
        if (mSize < rightSize) {
            return false;
        }
        return Traits::compare(mData + (mSize - rightSize), 
                               rhs.mData, rightSize) == 0;
    }

    XCONSTEXPR14 bool ends_with(CharT ch) const noexcept {
        return ends_with(basic_string_view{&ch, 1});
    }

    XCONSTEXPR14 bool ends_with(const CharT* str) const noexcept {
        return ends_with(basic_string_view{str});
    }

    XCONSTEXPR14
    size_type find(basic_string_view rhs, size_type pos1 = 0) const noexcept {
        if (mSize < rhs.mSize || pos1 + rhs.mSize > mSize) {
            return npos;
        }

        if (rhs.empty()) { // always matches
            return min(pos1, mSize);
        }

        for (size_type i = pos1; i <= mSize - rhs.mSize; ++i) {
            if (mData[i] == rhs[0]) { // matched the first element
                size_type j = 1;
                for (j = 1; j < rhs.mSize; ++j) {
                    if (mData[i + j] != rhs[j]) { // mismatched
                        break;
                    }
                }

                if (j == rhs.mSize) {
                    return i; // matched
                }
            }
        }

        return npos;
    }

    XCONSTEXPR14
    size_type find(CharT ch, size_type pos1 = 0) const noexcept {
        for (size_type i = pos1; i < mSize; ++i) {
            if (mData[i] == ch) {
                return i; // matched
            }
        }

        return npos;
    }

    XCONSTEXPR14
    size_type find(const CharT* str, size_type pos1,
                   size_type count2) const noexcept {
        return find(basic_string_view{str, count2}, pos1);
    }

    XCONSTEXPR14
    size_type find(const CharT* str, size_type pos1) const noexcept {
        return find(basic_string_view{str}, pos1);
    }

    XCONSTEXPR14
    size_type rfind(basic_string_view rhs,
                    size_type pos1 = npos) const noexcept {
        if (rhs.mSize == 0) { // always matches
            return min(pos1, mSize);
        }

        if (rhs.mSize > mSize) {
            return npos;
        }

        for (size_type i = min(pos1, mSize - rhs.mSize);
             i != static_cast<size_type>(-1); --i) {
            if (mData[i] == rhs[0]) { // matched the first element
                size_type j = 1;
                for (j = 1; j < rhs.mSize; ++j) {
                    if (mData[i + j] != rhs[j]) {
                        break;
                    }
                }

                if (j == rhs.mSize) {
                    return i;
                }
            }
        }

        return npos;
    }

    XCONSTEXPR14
    size_type rfind(CharT ch, size_type pos1 = npos) const noexcept {
        if (mSize == 0) {
            return npos;
        }

        for (int i = static_cast<int>(min(pos1, mSize - 1)); i >= 0; --i) {
            if (mData[i] == ch) {
                return i; // matched
            }
        }

        return npos;
    }

    XCONSTEXPR14 size_type rfind(const CharT* str, size_type pos1,
                                 size_type count2) const {
        return rfind(basic_string_view{str, count2}, pos1);
    }

    XCONSTEXPR14
    size_type rfind(const CharT* str, size_type pos1 = npos) const {
        return rfind(basic_string_view{str}, pos1);
    }

    static XCONSTEXPR14 size_type getStringLength(const_pointer str) noexcept {
        size_type len = Traits::length(str);
        return len;
    }

    static XCONSTEXPR14 int compareAux(const_pointer lhs, const size_type lsize,
                                       const_pointer rhs,
                                       const size_type rsize) noexcept {
        const int ans = Traits::compare(lhs, rhs, min(lsize, rsize));
        if (ans != 0) {
            return ans;
        }

        if (lsize < rsize) {
            return -1;
        }

        if (lsize > rsize) {
            return 1;
        }

        return 0;
    }

    constexpr void checkPosition(const size_type pos) const {
        if (pos > mSize) {
            xRange();
        }
    }

    constexpr void checkPostionEqual(const size_type pos) const {
        if (pos >= mSize) {
            xRange();
        }
    }

    [[noreturn]] static void xRange() {
        throw std::out_of_range("invalid string_base_view<CharT> out of range");
    }

    const_pointer mData;
    size_type mSize;
};

template <typename CharT, typename Traits>
constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT> rhs) noexcept {
    return lhs.size() == rhs.size() &&
           equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename CharT, typename Traits>
constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT> rhs) noexcept {
    return !(lhs == rhs);
}

template <typename CharT, typename Traits>
constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                         basic_string_view<CharT> rhs) noexcept {
    return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                   rhs.end());
}

template <typename CharT, typename Traits>
constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                         basic_string_view<CharT> rhs) noexcept {
    return rhs < lhs;
}

template <typename CharT, typename Traits>
constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT> rhs) noexcept {
    return !(rhs < lhs);
}

template <typename CharT, typename Traits>
constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT> rhs) noexcept {
    return !(lhs > rhs);
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           basic_string_view<CharT, Traits> sv) {
    os << sv.data();
    return os;
}

template <typename CharT, typename Traits>
struct hash<basic_string_view<CharT, Traits>> {
    using argument_type = basic_string_view<CharT, Traits>;
    using result_type = std::size_t;

    std::size_t operator()(const basic_string_view<CharT, Traits>& str) const
        noexcept {
        return tiny_stl::hashFNV(str.data(), str.size());
    }
};

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

#ifdef TINY_STL_CXX14
// changed operator""sv to operator""_sv
// in order to avoid the warning of MSVC:
// literal suffix identifiers that do not start with an underscore are reserved

namespace literals {

namespace string_view_literals {

constexpr string_view operator""_sv(const char* str, std::size_t len) noexcept {
    return string_view{str, len};
}

constexpr wstring_view operator""_sv(const wchar_t* str,
                                     std::size_t len) noexcept {
    return wstring_view{str, len};
}

constexpr u16string_view operator""_sv(const char16_t* str,
                                       std::size_t len) noexcept {
    return u16string_view{str, len};
}

constexpr u32string_view operator""_sv(const char32_t* str,
                                       std::size_t len) noexcept {
    return u32string_view{str, len};
}

} // namespace string_view_literals

} // namespace literals

#endif // TINY_STL_CXX14

} // namespace tiny_stl
