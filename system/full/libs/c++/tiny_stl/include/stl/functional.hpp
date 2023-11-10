// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "utility.hpp"

namespace tiny_stl {

template <typename UnaryPred>
class unary_negate {
private:
    UnaryPred functor;

public:
    using argument_type = typename UnaryPred::argument_type;
    using result_type = bool;

    constexpr explicit unary_negate(const UnaryPred& func) : functor(func) {
    }

    constexpr bool operator()(const argument_type& lhs) const {
        return (!functor(lhs));
    }
};

template <typename BinPred>
class binary_negate {
private:
    BinPred functor;

public:
    using first_argument_type = typename BinPred::first_argument_type;
    using second_argument_type = typename BinPred::second_argument_type;
    using result_type = bool;

    constexpr explicit binary_negate(const BinPred& func) : functor(func) {
    }

    constexpr bool operator()(const first_argument_type& lhs,
                              const second_argument_type& rhs) const {
        return (!functor(lhs, rhs));
    }
};

// operator&
template <typename T = void>
struct bit_and {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs & rhs);
    }
};

template <>
struct bit_and<void> {
    using is_transparent = int;

    // C++ 11
    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) & tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) & tiny_stl::forward<T2>(rhs));
    }

    // or C++ 14
    // template <typename T1, typename T2>
    // constexpr decltype(auto) operator()(T1&& lhs, T2&& rhs) const
    // {
    //     return (tiny_stl::forward<T1>(lhs) & tiny_stl::forward<T2>(rhs));
    // }
    //
    // The same below
};

// operator|
template <typename T = void>
struct bit_or {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs | rhs);
    }
};

template <>
struct bit_or<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) | tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) | tiny_stl::forward<T2>(rhs));
    }
};

// operator^
template <typename T = void>
struct bit_xor {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs ^ rhs);
    }
};

template <>
struct bit_xor<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) ^ tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) ^ tiny_stl::forward<T2>(rhs));
    }
};

// operator&&
template <typename T = void>
struct logical_and {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs && rhs);
    }
};

template <>
struct logical_and<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) && tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) && tiny_stl::forward<T2>(rhs));
    }
};

// operator||
template <typename T = void>
struct logical_or {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs || rhs);
    }
};

template <>
struct logical_or<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) || tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) || tiny_stl::forward<T2>(rhs));
    }
};

// operator!
template <typename T = void>
struct logical_not {
    using argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs) const {
        return (!lhs);
    }
};

template <>
struct logical_not<void> {
    using is_transparent = int;

    template <typename T>
    constexpr auto operator()(T&& lhs) const
        -> decltype(!tiny_stl::forward<T>(lhs)) {
        return (!tiny_stl::forward<T>(lhs));
    }
};

// operator+
template <typename T = void>
struct plus {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return lhs + rhs;
    }
};

template <>
struct plus<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) + tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) + tiny_stl::forward<T2>(rhs));
    }
};

// binary operator-
template <typename T = void>
struct minus {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return lhs - rhs;
    }
};

template <>
struct minus<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) - tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) - tiny_stl::forward<T2>(rhs));
    }
};

// operator*
template <typename T = void>
struct multiplies {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return lhs * rhs;
    }
};

template <>
struct multiplies<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) * tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) * tiny_stl::forward<T2>(rhs));
    }
};

// operator/
template <typename T = void>
struct divides {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return lhs / rhs;
    }
};

template <>
struct divides<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) / tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) / tiny_stl::forward<T2>(rhs));
    }
};

// operator%
template <typename T = void>
struct modulus {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = T;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs % rhs);
    }
};

template <>
struct modulus<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) % tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) % tiny_stl::forward<T2>(rhs));
    }
};

// unary operator-
template <typename T = void>
struct negate {
    using argument_type = T;
    using result_type = T;

    constexpr T operator()(const T& lhs) const {
        return (-lhs);
    }
};

template <>
struct negate<void> {
    using is_transparent = int;

    template <typename T>
    constexpr auto operator()(T&& lhs) const
        -> decltype(-tiny_stl::forward<T>(lhs)) {
        return (-tiny_stl::forward<T>(lhs));
    }
};

// operator==
template <typename T = void>
struct equal_to {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs == rhs);
    }
};

template <>
struct equal_to<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) == tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) == tiny_stl::forward<T2>(rhs));
    }
};

// operator!=
template <typename T = void>
struct not_equal_to {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs != rhs);
    }
};

template <>
struct not_equal_to<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) != tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) != tiny_stl::forward<T2>(rhs));
    }
};

// operator<
template <typename T = void>
struct less {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs < rhs);
    }
};

template <>
struct less<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) < tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) < tiny_stl::forward<T2>(rhs));
    }
};

// operator<=
template <typename T = void>
struct less_equal {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs <= rhs);
    }
};

template <>
struct less_equal<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) <= tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) <= tiny_stl::forward<T2>(rhs));
    }
};

// operator>
template <typename T = void>
struct greater {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs > rhs);
    }
};

template <>
struct greater<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) > tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) > tiny_stl::forward<T2>(rhs));
    }
};

// operator>=
template <typename T = void>
struct greater_equal {
    using first_argument_type = T;
    using second_argument_type = T;
    using result_type = bool;

    constexpr result_type operator()(const T& lhs, const T& rhs) const {
        return (lhs >= rhs);
    }
};

template <>
struct greater_equal<void> {
    using is_transparent = int;

    template <typename T1, typename T2>
    constexpr auto operator()(T1&& lhs, T2&& rhs) const
        -> decltype(tiny_stl::forward<T1>(lhs) >= tiny_stl::forward<T2>(rhs)) {
        return (tiny_stl::forward<T1>(lhs) >= tiny_stl::forward<T2>(rhs));
    }
};

template <typename T>
inline std::size_t hashFNV(const T* p, std::size_t count) noexcept {
    static_assert(is_arithmetic_v<T>, "T must be arithmetic type");
    std::size_t ret = 2166136261U;
    for (std::size_t i = 0u; i < count; ++i) {
        ret ^= static_cast<std::size_t>(p[i]);
        ret *= 16777619U;
    }

    return ret;
}

template <typename Key>
struct hash {
    using argument_type = Key;
    using result_type = std::size_t;

    std::size_t operator()(const Key& key) const noexcept {
        return hashFNV(&reinterpret_cast<const unsigned char&>(key),
                       sizeof(Key));
    }
};

} // namespace tiny_stl