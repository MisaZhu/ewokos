// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "utility.hpp"

namespace tiny_stl {

template <typename... Ts>
class tuple;

template <>
class tuple<> {
public:
    constexpr tuple() noexcept {
    }

    constexpr tuple(const tuple&) noexcept {
    }

    constexpr tuple(tuple&&) noexcept {
    }

    tuple& operator=(const tuple&) noexcept {
        return *this;
    }

    tuple& operator=(tuple&&) noexcept {
        return *this;
    }

    void swap(tuple&) noexcept {
    }

public:
    constexpr bool equal(const tuple&) const noexcept {
        return true;
    }

    constexpr bool less(const tuple&) const noexcept {
        return false;
    }
};

template <typename Head, typename... Tail>
class tuple<Head, Tail...> : private tuple<Tail...> {
public:
    using First = Head;
    using Base = tuple<Tail...>;

    static constexpr std::size_t size = 1 + sizeof...(Tail);

protected:
    Head mHead;

public:
    // (1)
    template <enable_if_t<is_default_constructible<Head>::value, int> = 0>
    explicit tuple() : Base(), mHead() {
    }

    // (2)
    template <enable_if_t<is_copy_constructible<Head>::value, int> = 0>
    explicit constexpr tuple(Head h, Tail... t) : Base(t...), mHead(h) {
    }

    // (3)
    template <typename H, typename... T,
              enable_if_t<is_constructible<Head, H&&>::value &&
                              sizeof...(Tail) == sizeof...(T),
                          int> = 0>
    explicit constexpr tuple(H&& h, T&&... t)
        : mHead(tiny_stl::forward<H>(h)), Base(tiny_stl::forward<T>(t)...) {
    }

    // (4)
    template <typename H, typename... T,
              enable_if_t<is_constructible<Head, const H&>::value &&
                              sizeof...(Tail) == sizeof...(T),
                          int> = 0>
    constexpr tuple(const tuple<H, T...>& rhs)
        : mHead(rhs.get_head()), Base(rhs.get_tail()) {
    }

    // (5)
    template <typename H, typename... T,
              enable_if_t<is_constructible<Head, H&&>::value &&
                              sizeof...(Tail) == sizeof...(T),
                          int> = 0>
    constexpr tuple(tuple<H, T...>&& rhs)
        : mHead(tiny_stl::forward<H&&>(rhs.get_head())),
          Base(tiny_stl::forward<Base&&>(rhs.get_tail())) {
    }

    // (6)
    template <typename U1, typename U2,
              enable_if_t<is_constructible<Head, const U1&>::value &&
                              sizeof...(Tail) == 1,
                          int> = 0>
    constexpr tuple(const pair<U1, U2>& p) : mHead(p.first), Base(p.second) {
    }

    // (7)
    template <
        typename U1, typename U2,
        enable_if_t<is_constructible<Head, U1&&>::value && sizeof...(Tail) == 1,
                    int> = 0>
    constexpr tuple(pair<U1, U2>&& p)
        : mHead(tiny_stl::forward<U1&&>(p.first)),
          Base(tiny_stl::forward<U2&&>(p.second)) {
    }

    // (8)
    tuple(const tuple&) = default;
    // (9)
    tuple(tuple&&) = default;

    tuple& operator=(const tuple& rhs) {
        get_head() = rhs.get_head();
        get_tail() = rhs.get_tail();

        return *this;
    }

    tuple& operator=(tuple&& rhs) noexcept(
        std::is_nothrow_move_assignable<Head>::value&&
            std::is_nothrow_move_assignable<Base>::value) {
        get_head() = tiny_stl::forward<Head>(rhs.get_head());
        get_tail() = tiny_stl::forward<Base>(rhs.get_tail());

        return *this;
    }

    template <typename... Ts>
    tuple& operator=(const tuple<Ts...>& rhs) {
        get_head() = rhs.get_head();
        get_tail() = rhs.get_tail();

        return *this;
    }

    template <typename... Ts>
    tuple& operator=(tuple<Ts...>&& rhs) {
        get_head() =
            tiny_stl::forward<typename tuple<Ts...>::First>(rhs.get_head());
        get_tail() =
            tiny_stl::forward<typename tuple<Ts...>::Base>(rhs.get_tail());

        return *this;
    }

    template <typename U1, typename U2>
    tuple& operator=(const pair<U1, U2>& rhs) {
        get_head() = rhs.first;
        get_tail().get_head() = rhs.second;

        return *this;
    }

    template <typename U1, typename U2>
    tuple& operator=(pair<U1, U2>&& rhs) {
        get_head() = tiny_stl::forward<U1>(rhs.first);
        get_tail().get_head() = tiny_stl::forward<U2>(rhs.second);

        return *this;
    }

    void swap(tuple& rhs) noexcept(
        conjunction<is_nothrow_swappable<Head>,
                    is_nothrow_swappable<Tail>...>::value) {
        tiny_stl::swapADL(get_head(), rhs.get_head());
        Base::swap(rhs.get_tail());
    }

    Head& get_head() {
        return mHead;
    }

    const Head& get_head() const {
        return mHead;
    }

    Base& get_tail() {
        return *this;
    }

    const Base& get_tail() const {
        return *this;
    }

    template <typename... Us>
    constexpr bool equal(const tuple<Us...>& rhs) const {
        static_assert(size == sizeof...(Us), "tuple with different size");

        return get_head() == rhs.get_head() && Base::equal(rhs.get_tail());
    }

    template <typename... Us>
    constexpr bool less(const tuple<Us...>& rhs) const {
        static_assert(size == sizeof...(Us), "tuple with different size");

        return get_head() < rhs.get_head() ||
               (!(rhs.get_head() < get_head()) && Base::less(rhs.get_tail()));
    }
};

template <typename... Ts>
inline void swap(tuple<Ts...>& lhs,
                 tuple<Ts...>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename... Args>
inline auto make_tuple(Args... args) {
    return tuple<UnRefWrap<Args>...>(tiny_stl::forward<Args>(args)...);
}

template <typename... Ts>
constexpr tuple<Ts&...> tie(Ts&... ts) noexcept {
    using TupleType = tuple<Ts&...>;
    return TupleType(ts...);
}

template <typename... Ts>
constexpr tuple<Ts&&...> forward_as_tuple(Ts&&... ts) noexcept {
    using TupleType = tuple<Ts&&...>;
    return TupleType(tiny_stl::forward<Ts>(ts)...);
}

template <std::size_t Idx, typename... Ts>
constexpr tuple_element_t<Idx, tuple<Ts...>>& get(tuple<Ts...>& t) noexcept {
    using TupleType = typename tuple_element<Idx, tuple<Ts...>>::TupleType;
    return reinterpret_cast<TupleType&>(t).get_head();
}

template <std::size_t Idx, typename... Ts>
constexpr tuple_element_t<Idx, tuple<Ts...>>&& get(tuple<Ts...>&& t) noexcept {
    using TupleType = typename tuple_element<Idx, tuple<Ts...>>::TupleType;
    using RetType = tuple_element_t<Idx, tuple<Ts...>>&&;

    return tiny_stl::forward<RetType>(
        reinterpret_cast<TupleType&>(t).get_head());
}

template <std::size_t Idx, typename... Ts>
constexpr const tuple_element_t<Idx, tuple<Ts...>>&
get(const tuple<Ts...>& t) noexcept {
    using TupleType = typename tuple_element<Idx, tuple<Ts...>>::TupleType;
    return reinterpret_cast<TupleType&>(t).get_head();
}

template <std::size_t Idx, typename... Ts>
constexpr const tuple_element_t<Idx, tuple<Ts...>>&&
get(const tuple<Ts...>&& t) noexcept {
    using TupleType = typename tuple_element<Idx, tuple<Ts...>>::TupleType;
    using RetType = const tuple_element_t<Idx, tuple<Ts...>>&&;

    return tiny_stl::forward<RetType>(
        reinterpret_cast<TupleType&>(t).get_head());
}

template <typename... Ts, typename... Us>
constexpr bool operator==(const tuple<Ts...>& lhs, const tuple<Us...>& rhs) {
    return lhs.equal(rhs);
}

template <typename... Ts, typename... Us>
constexpr bool operator!=(const tuple<Ts...>& lhs, const tuple<Us...>& rhs) {
    return !(lhs == rhs);
}

template <typename... Ts, typename... Us>
constexpr bool operator<(const tuple<Ts...>& lhs, const tuple<Us...>& rhs) {
    return lhs.less(rhs);
}

template <typename... Ts, typename... Us>
constexpr bool operator>(const tuple<Ts...>& lhs, const tuple<Us...>& rhs) {
    return rhs < lhs;
}

template <typename... Ts, typename... Us>
constexpr bool operator<=(const tuple<Ts...>& lhs, const tuple<Us...>& rhs) {
    return !(rhs < lhs);
}

template <typename... Ts, typename... Us>
constexpr bool operator>=(const tuple<Ts...>& lhs, const tuple<Us...>& rhs) {
    return !(lhs < rhs);
}

template <typename... Ts, typename Alloc>
struct uses_allocator<tuple<Ts...>, Alloc> : true_type {};

} // namespace tiny_stl
