// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "deque.hpp"
#include "vector.hpp"

namespace tiny_stl {

template <typename T, typename Container = tiny_stl::deque<T>>
class queue {
public:
    using container_type = Container;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;

public:
    static_assert(is_same<T, value_type>::value,
                  "container_type::value_type error");

private:
    container_type cont;

public:
    // (1)
    explicit queue(const container_type& c) : cont(c) {
    }

    // (2)
    explicit queue(container_type&& c = container_type())
        : cont(tiny_stl::move(c)) {
    }

    // (3)
    queue(const queue& rhs) : cont(rhs.cont) {
    }

    // (4)
    queue(queue&& rhs) : cont(tiny_stl::move(rhs.cont)) {
    }

    // (5)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    explicit queue(const Alloc& alloc) : cont(alloc) {
    }

    // (6)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    queue(const container_type& c, const Alloc& alloc) : cont(c, alloc) {
    }

    // (7)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    queue(container_type&& c, const Alloc& alloc)
        : cont(tiny_stl::move(c), alloc) {
    }

    // (8)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    queue(const queue& rhs, const Alloc& alloc) : cont(rhs.cont, alloc) {
    }

    // (9)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    queue(queue&& rhs, const Alloc& alloc)
        : cont(tiny_stl::move(rhs.cont), alloc) {
    }

    reference front() {
        return cont.front();
    }

    const_reference front() const {
        return cont.front();
    }

    reference back() {
        return cont.back();
    }

    const_reference back() const {
        return cont.back();
    }

    bool empty() const {
        return cont.empty();
    }

    size_type size() const {
        return cont.size();
    }

    void push(const value_type& val) {
        cont.push_back(val);
    }

    void push(value_type&& val) {
        cont.push_back(tiny_stl::move(val));
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        cont.emplace_back(tiny_stl::forward<Args>(args)...);
    }

    void pop() {
        cont.pop_front();
    }

    void swap(queue& rhs) noexcept(is_nothrow_swappable<Container>::value) {
        swapADL(cont, rhs.cont);
    }

    const container_type& getContainer() const {
        return this->cont;
    }
}; // class queue<T, Container>

template <typename T, typename Container>
inline bool operator==(const queue<T, Container>& lhs,
                       const queue<T, Container>& rhs) {
    return lhs.getContainer() == rhs.getContainer();
}

template <typename T, typename Container>
inline bool operator!=(const queue<T, Container>& lhs,
                       const queue<T, Container>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Container>
inline bool operator<(const queue<T, Container>& lhs,
                      const queue<T, Container>& rhs) {
    return lhs.getContainer() < rhs.getContainer();
}

template <typename T, typename Container>
inline bool operator>(const queue<T, Container>& lhs,
                      const queue<T, Container>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Container>
inline bool operator<=(const queue<T, Container>& lhs,
                       const queue<T, Container>& rhs) {
    return !(rhs < lhs);
}

template <typename T, typename Container>
inline bool operator>=(const queue<T, Container>& lhs,
                       const queue<T, Container>& rhs) {
    return !(lhs < rhs);
}

template <typename T, typename Container>
inline void swap(queue<T, Container>& lhs,
                 queue<T, Container>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename T, typename Container, typename Alloc>
struct uses_allocator<queue<T, Container>, Alloc>
    : uses_allocator<Container, Alloc>::type {};

template <typename T, typename Container = tiny_stl::vector<T>,
          typename Compare = tiny_stl::less<typename Container::value_type>>
class priority_queue {
public:
    using container_type = Container;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using value_compare = Compare;

public:
    static_assert(is_same<T, value_type>::value,
                  "container_type::value_type error");

private:
    value_compare comp;
    container_type cont;

public:
    // (1)
    priority_queue(const value_compare& cmp, const container_type& c)
        : comp(cmp), cont(c) {
        tiny_stl::make_heap(cont.begin(), cont.end(), comp);
    }

    // (2)
    explicit priority_queue(const value_compare& cmp = value_compare{},
                            container_type&& c = container_type{})
        : comp(cmp), cont(tiny_stl::move(c)) {
        tiny_stl::make_heap(cont.begin(), cont.end(), comp);
    }

    // (3)
    priority_queue(const priority_queue& rhs) : comp(rhs.comp), cont(rhs.cont) {
    }

    // (4)
    priority_queue(priority_queue&& rhs)
        : comp(tiny_stl::move(rhs.comp)), cont(tiny_stl::move(rhs.cont)) {
    }

    // (5)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    explicit priority_queue(const Alloc& alloc) : comp(), cont(alloc) {
    }

    // (6)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    priority_queue(const value_compare& cmp, const Alloc& alloc)
        : comp(cmp), cont(alloc) {
    }

    // (7)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    priority_queue(const value_compare& cmp, const container_type& c,
                   const Alloc& alloc)
        : comp(cmp), cont(c, alloc) {
        tiny_stl::make_heap(cont.begin(), cont.end(), comp);
    }

    // (8)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    priority_queue(const value_compare& cmp, container_type&& c,
                   const Alloc& alloc)
        : comp(cmp), cont(tiny_stl::move(c), alloc) {
        tiny_stl::make_heap(cont.begin(), cont.end(), comp);
    }

    // (9)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    priority_queue(const priority_queue& rhs, const Alloc& alloc)
        : comp(rhs.comp), cont(rhs.cont, alloc) {
    }

    // (10)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    priority_queue(priority_queue&& rhs, const Alloc& alloc)
        : comp(tiny_stl::move(rhs.comp)),
          cont(tiny_stl::move(rhs.cont), alloc) {
    }

    // (11)
    template <typename InIter>
    priority_queue(InIter first, InIter last, const value_compare& cmp,
                   const container_type& c)
        : comp(cmp), cont(c) {
        cont.insert(cont.end(), first, last);
        tiny_stl::make_heap(cont.begin(), cont.end(), comp);
    }

    // (12)
    template <typename InIter>
    priority_queue(InIter first, InIter last,
                   const value_compare& cmp = value_compare{},
                   container_type&& c = container_type{})
        : comp(cmp), cont(tiny_stl::move(c)) {
        cont.insert(cont.end(), first, last);
        tiny_stl::make_heap(cont.begin(), cont.end(), comp);
    }

    const_reference top() const {
        return cont.front();
    }

    bool empty() const {
        return cont.empty();
    }

    size_type size() const {
        return cont.size();
    }

    void push(const value_type& val) {
        cont.push_back(val);
        tiny_stl::push_heap(cont.begin(), cont.end(), comp);
    }

    void push(value_type&& val) {
        cont.push_back(tiny_stl::move(val));
        tiny_stl::push_heap(cont.begin(), cont.end(), comp);
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        cont.emplace_back(tiny_stl::forward<Args>(args)...);
        tiny_stl::push_heap(cont.begin(), cont.end(), comp);
    }

    void pop() {
        tiny_stl::pop_heap(cont.begin(), cont.end(), comp);
        cont.pop_back();
    }

    void swap(priority_queue& rhs) noexcept(
        is_nothrow_swappable<Container>::value&&
            is_nothrow_swappable<Compare>::value) {
        swapADL(comp, rhs.comp);
        swapADL(cont, rhs.cont);
    }
}; // class priority_queue<T, Container>

template <typename T, typename Container, typename Compare>
void swap(priority_queue<T, Container, Compare>& lhs,
          priority_queue<T, Container, Compare>&
              rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename T, typename Container, typename Compare, typename Alloc>
struct uses_allocator<priority_queue<T, Container, Compare>, Alloc>
    : tiny_stl::uses_allocator<Container, Alloc>::type {};

} // namespace tiny_stl
