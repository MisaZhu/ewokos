// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "deque.hpp"

namespace tiny_stl {

template <typename T, typename Container = tiny_stl::deque<T>>
class stack {
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
    explicit stack(const container_type& c) : cont(c) {
    }

    // (2)
    explicit stack(container_type&& cout = container_type())
        : cont(tiny_stl::move(cout)) {
    }

    // (3)
    stack(const stack& rhs) : cont(rhs.cont) {
    }

    // (4)
    stack(stack&& rhs) : cont(tiny_stl::move(rhs)) {
    }

    // (5)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    explicit stack(const Alloc& alloc) : cont(alloc) {
    }

    // (6)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    stack(const container_type& c, const Alloc& alloc) : cont(c, alloc) {
    }

    // (7)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    stack(container_type&& c, const Alloc& alloc)
        : cont(tiny_stl::move(c), alloc) {
    }

    // (8)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    stack(const stack& rhs, const Alloc& alloc) : cont(rhs.cont, alloc) {
    }

    // (9)
    template <typename Alloc, typename = enable_if_t<
                                  uses_allocator<container_type, Alloc>::value>>
    stack(stack&& rhs, const Alloc& alloc)
        : cont(tiny_stl::move(rhs.cont), alloc) {
    }

    // implicitly declared operator=()
    // ...

public:
    reference top() {
        return cont.back();
    }

    const_reference top() const {
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

    void pop() {
        cont.pop_back();
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        cont.emplace_back(tiny_stl::forward<Args>(args)...);
    }

    void swap(stack& rhs) noexcept(is_nothrow_swappable<Container>::value) {
        swapADL(this->cont, rhs.cont);
    }

    const container_type& getContainer() const {
        return this->cont;
    }
}; // class stack<T, Container>

template <typename T, typename Container>
inline bool operator==(const stack<T, Container>& lhs,
                       const stack<T, Container>& rhs) {
    return lhs.getContainer() == rhs.getContainer();
}

template <typename T, typename Container>
inline bool operator!=(const stack<T, Container>& lhs,
                       const stack<T, Container>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Container>
inline bool operator<(const stack<T, Container>& lhs,
                      const stack<T, Container>& rhs) {
    return lhs.getContainer() < rhs.getContainer();
}

template <typename T, typename Container>
inline bool operator>(const stack<T, Container>& lhs,
                      const stack<T, Container>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Container>
inline bool operator<=(const stack<T, Container>& lhs,
                       const stack<T, Container>& rhs) {
    return !(rhs < lhs);
}

template <typename T, typename Container>
inline bool operator>=(const stack<T, Container>& lhs,
                       const stack<T, Container>& rhs) {
    return !(lhs < rhs);
}

template <typename T, typename Container>
inline void swap(stack<T, Container>& lhs,
                 stack<T, Container>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename T, typename Container, typename Alloc>
struct uses_allocator<stack<T, Container>, Alloc>
    : uses_allocator<Container, Alloc>::type {};

} // namespace tiny_stl
