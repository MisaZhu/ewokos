// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "utility.hpp"

#include <cstddef>

namespace tiny_stl {

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

template <typename T, typename Distance>
struct input_iterator {
    using iterator_category = input_iterator_tag;
    using value_type = T;
    using difference_type = Distance;
    using pointer = T*;
    using reference = T&;
};

struct output_iterator {
    using iterator_category = input_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
};

template <typename T, typename Distance>
struct forward_iterator {
    using iterator_category = forward_iterator_tag;
    using value_type = T;
    using difference_type = Distance;
    using pointer = T*;
    using reference = T&;
};

template <typename T, typename Distance>
struct bidirectional_iterator {
    using iterator_category = bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = Distance;
    using pointer = T*;
    using reference = T&;
};

template <typename T, typename Distance>
struct random_access_iterator {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using difference_type = Distance;
    using pointer = T*;
    using reference = T&;
};

template <typename Category, typename T, typename Distance = std::ptrdiff_t,
          typename Pointer = T*, typename Reference = T&>
struct iterator {
    using iterator_category = Category;
    using value_type = T;
    using difference_type = Distance;
    using pointer = Pointer;
    using reference = Reference;
};

template <typename, typename = void>
struct IteratorTraitsBase {};

template <typename Iter>
struct IteratorTraitsBase<
    Iter, void_t<typename Iter::iterator_category, typename Iter::value_type,
                 typename Iter::difference_type, typename Iter::pointer,
                 typename Iter::reference>> {
    using iterator_category = typename Iter::iterator_category;
    using value_type = typename Iter::value_type;
    using difference_type = typename Iter::difference_type;
    using pointer = typename Iter::pointer;
    using reference = typename Iter::reference;
};

// pointer to object
template <typename T, bool = is_object<T>::value>
struct IteratorTraitsPointerBase {
    using iterator_category = random_access_iterator_tag;
    using value_type = remove_cv_t<T>;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
};

template <typename Iter>
struct iterator_traits : IteratorTraitsBase<Iter> {};

template <typename T>
struct iterator_traits<T*> : IteratorTraitsPointerBase<T> {};

template <typename T, typename = void>
struct is_iterator : false_type {};

template <typename T>
struct is_iterator<T, void_t<typename iterator_traits<T>::iterator_category>>
    : true_type {};

template <typename T>
constexpr bool is_iterator_v = is_iterator<T>::value;

namespace details {

template <typename Iter>
inline typename iterator_traits<Iter>::category iterator_category(const Iter&) {
    using category = typename iterator_traits<Iter>::iterator_category;
    return category{};
}

template <typename Iter>
inline typename iterator_traits<Iter>::difference_type*
distance_type(const Iter&) {
    return static_cast<typename iterator_traits<Iter>::difference*>(0);
}

template <typename Iter>
inline typename iterator_traits<Iter>::value_type* value_type(const Iter&) {
    return static_cast<typename iterator_traits<Iter>::value_type*>(0);
}

template <typename Iter>
using IteratorValueType = typename iterator_traits<Iter>::value_type;

template <typename Iter>
constexpr Iter operator_arrow(Iter target, true_type /*is a pointer*/) {
    return target;
}

template <typename Iter>
constexpr decltype(auto) operator_arrow(Iter&& target,
                                        false_type /*is not a pointer*/) {
    return (tiny_stl::forward<Iter>(target).operator->());
}

} // namespace details

//  rend                           rbegin
// |v |p                            |v |p     v is value, p is position
// v  v                             v  v
//    E  E  E  E  E  E  E  E  E  E  E
//    ^                                ^
//    |                                |
//  begin                             end

template <typename Iter>
class reverse_iterator {
protected:
    Iter current;

public:
    using iterator_category = typename iterator_traits<Iter>::iterator_category;
    using value_type = typename iterator_traits<Iter>::value_type;
    using difference_type = typename iterator_traits<Iter>::difference_type;
    using pointer = typename iterator_traits<Iter>::pointer;
    using reference = typename iterator_traits<Iter>::reference;
    using iterator_type = Iter;
    using Self = reverse_iterator<Iter>;

public:
    constexpr reverse_iterator() : current() {
    }
    constexpr explicit reverse_iterator(iterator_type x) : current(x) {
    }

    template <typename Other>
    constexpr reverse_iterator(const reverse_iterator<Other>& rhs)
        : current(rhs.base()) {
    }

    template <typename Other>
    constexpr Self& operator=(const reverse_iterator<Other>& rhs) {
        current = rhs.base();
        return *this;
    }

    constexpr Iter base() const {
        return current;
    }

    constexpr reference operator*() const {
        Iter tmp = current;
        return (*--tmp);
    }

    constexpr pointer operator->() const {
        Iter tmp = current;
        --tmp;
        return (operator_arrow(tmp, is_pointer<Iter>()));
    }

    constexpr Self& operator++() {
        --current;
        return *this;
    }

    constexpr Self operator++(int) {
        Self tmp = *this;
        --current;
        return tmp;
    }

    constexpr Self& operator--() {
        ++current;
        return *this;
    }

    constexpr Self operator--(int) {
        Self tmp = *this;
        ++current;
        return tmp;
    }

    // only random-access iterators

    constexpr Self& operator+=(difference_type n) {
        current -= n;
        return *this;
    }

    constexpr Self operator+(difference_type n) const {
        return Self(current - n);
    }

    constexpr Self& operator-=(difference_type n) {
        current += n;
        return *this;
    }

    constexpr Self operator-(difference_type n) const {
        return Self(current + n);
    }

    constexpr reference operator[](difference_type n) const {
        return *(*this + n);
    }
}; // class reverse_iterator<Iter>

template <typename Iter1, typename Iter2>
constexpr auto operator+(typename reverse_iterator<Iter1>::difference_type n,
                         const reverse_iterator<Iter2>& reIter) {
    return reIter + n;
}

template <typename Iter1, typename Iter2>
constexpr auto operator-(const reverse_iterator<Iter1>& lhs,
                         const reverse_iterator<Iter2>& rhs)
    -> decltype(rhs.base() - lhs.base()) {
    return rhs.base() - lhs.base();
}

template <typename Iter1, typename Iter2>
constexpr bool operator==(const reverse_iterator<Iter1>& lhs,
                          const reverse_iterator<Iter2>& rhs) {
    return lhs.base() == rhs.base();
}

template <typename Iter1, typename Iter2>
constexpr bool operator!=(const reverse_iterator<Iter1>& lhs,
                          const reverse_iterator<Iter2>& rhs) {
    return (!(lhs == rhs));
}

template <typename Iter1, typename Iter2>
constexpr bool operator<(const reverse_iterator<Iter1>& lhs,
                         const reverse_iterator<Iter2>& rhs) {
    // reverse iterator, so rhs.base() < lhs.base()
    return (rhs.base() < lhs.base());
}

template <typename Iter1, typename Iter2>
constexpr bool operator>(const reverse_iterator<Iter1>& lhs,
                         const reverse_iterator<Iter2>& rhs) {
    return rhs < lhs;
}

template <typename Iter1, typename Iter2>
constexpr bool operator<=(const reverse_iterator<Iter1>& lhs,
                          const reverse_iterator<Iter2>& rhs) {
    return (!(rhs < lhs));
}

template <typename Iter1, typename Iter2>
constexpr bool operator>=(const reverse_iterator<Iter1>& lhs,
                          const reverse_iterator<Iter2>& rhs) {
    return (!(lhs < rhs));
}

template <typename Iter>
constexpr reverse_iterator<Iter> make_reverse_iterator(Iter iter) {
    return reverse_iterator<Iter>(iter);
}

template <typename Iter>
class move_iterator {
protected:
    Iter current;

public:
    using iterator_type = Iter;
    using iterator_category = typename iterator_traits<Iter>::iterator_category;
    using value_type = typename iterator_traits<Iter>::value_type;
    using difference_type = typename iterator_traits<Iter>::difference_type;
    using pointer = Iter;
    using reference = value_type&&;

    constexpr move_iterator() : current() {
    }

    constexpr explicit move_iterator(Iter iter) : current(iter) {
    }

    template <typename Other>
    constexpr move_iterator(const move_iterator<Other>& rhs)
        : current(rhs.current) {
    }

    template <typename Other>
    constexpr move_iterator& operator=(const move_iterator<Other>& rhs) {
        current = rhs.current;
        return *this;
    }

    constexpr Iter base() const {
        return current;
    }

    constexpr reference operator*() const {
        return static_cast<reference>(*current);
    }

    constexpr pointer operator->() const {
        return current;
    }

    constexpr reference operator[](difference_type off) const {
        return tiny_stl::move(current[off]);
    }

    constexpr move_iterator& operator++() {
        ++current;
        return *this;
    }

    constexpr move_iterator operator++(int) {
        move_iterator tmp = *this;
        ++current;
        return tmp;
    }

    constexpr move_iterator& operator--() {
        --current;
        return *this;
    }

    constexpr move_iterator operator--(int) {
        move_iterator tmp = *this;
        --current;
        return tmp;
    }

    constexpr move_iterator operator+(difference_type offset) {
        return move_iterator(current + offset);
    }

    constexpr move_iterator operator-(difference_type offset) {
        return move_iterator(current - offset);
    }

    constexpr move_iterator& operator+=(difference_type offset) {
        current += offset;
        return *this;
    }

    constexpr move_iterator& operator-=(difference_type offset) {
        current -= offset;
        return *this;
    }
}; // class move_iterator<Iter>

template <typename Iter1, typename Iter2>
constexpr bool operator==(const move_iterator<Iter1>& lhs,
                          const move_iterator<Iter2>& rhs) {
    return lhs.base() == rhs.base();
}

template <typename Iter1, typename Iter2>
constexpr bool operator!=(const move_iterator<Iter1>& lhs,
                          const move_iterator<Iter2>& rhs) {
    return !(lhs == rhs);
}

template <typename Iter1, typename Iter2>
constexpr bool operator<(const move_iterator<Iter1>& lhs,
                         const move_iterator<Iter2>& rhs) {
    return lhs.base() < rhs.base();
}

template <typename Iter1, typename Iter2>
constexpr bool operator>(const move_iterator<Iter1>& lhs,
                         const move_iterator<Iter2>& rhs) {
    return rhs < lhs;
}

template <typename Iter1, typename Iter2>
constexpr bool operator<=(const move_iterator<Iter1>& lhs,
                          const move_iterator<Iter2>& rhs) {
    return !(rhs < lhs);
}

template <typename Iter1, typename Iter2>
constexpr bool operator>=(const move_iterator<Iter1>& lhs,
                          const move_iterator<Iter2>& rhs) {
    return !(lhs < rhs);
}

// iter + offset
template <typename Iter>
constexpr move_iterator<Iter>
operator+(typename move_iterator<Iter>::difference_type offset,
          const move_iterator<Iter>& iter) {
    return iter + offset;
}

// iter1 - iter2
template <typename Iter1, typename Iter2>
constexpr auto operator-(const move_iterator<Iter1>& lhs,
                         const move_iterator<Iter2>& rhs)
    -> decltype(lhs.base() - rhs.base()) {
    return lhs.base() - rhs.base();
}

template <typename Iter>
constexpr move_iterator<Iter> make_move_iterator(Iter iter) {
    return move_iterator<Iter>(iter);
}

namespace details {

template <typename InIter, typename Distance>
inline void advanceAux(InIter& iter, Distance n, input_iterator_tag) {
    while (n--)
        ++iter;
}

template <typename FwdIter, typename Distance>
inline void advanceAux(FwdIter& iter, Distance n, forward_iterator_tag) {
    advanceAux(iter, n, input_iterator_tag{});
}

template <typename BidirectionalIterator, typename Distance>
inline void advanceAux(BidirectionalIterator& iter, Distance n,
                       bidirectional_iterator_tag) {
    if (n >= 0)
        while (n--)
            ++iter;
    else
        while (n++)
            --iter;
}

template <typename RandomAccessIterator, typename Distance>
inline void advanceAux(RandomAccessIterator& iter, Distance n,
                       random_access_iterator_tag) {
    iter += n;
}

template <typename InIter>
inline typename iterator_traits<InIter>::difference_type
distanceAux(InIter first, InIter last, input_iterator_tag) {
    typename iterator_traits<InIter>::difference_type n = 0;
    while (first != last) {
        ++first;
        ++n;
    }
    return n;
}

template <typename RandomAccessIterator>
inline typename iterator_traits<RandomAccessIterator>::difference_type
distanceAux(RandomAccessIterator first, RandomAccessIterator last,
            random_access_iterator_tag) {
    return last - first;
}

} // namespace details

template <typename InIter, typename Distance>
inline void advance(InIter& iter, Distance n) {
    details::advanceAux(
        iter, n,
        typename iterator_traits<remove_const_t<InIter>>::iterator_category{});
}

template <typename InIter>
inline typename iterator_traits<InIter>::difference_type distance(InIter first,
                                                                  InIter last) {
    using category = typename iterator_traits<InIter>::iterator_category;
    return details::distanceAux(first, last, category{});
}

template <typename InIter>
constexpr InIter next(InIter iter,
                      typename iterator_traits<InIter>::difference_type n = 1) {
    tiny_stl::advance(iter, n);
    return iter;
}

template <typename Iter>
using IterDiffType = typename iterator_traits<Iter>::difference_type;

} // namespace tiny_stl
