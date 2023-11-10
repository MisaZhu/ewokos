// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <initializer_list>

#include "memory.h"

namespace tiny_stl {

// iterator e.g.:
//                           first    cur      last
//                            |        |        |
//                            v        v        v
//                            ___________________
//                    /----> |___________________ buffer(related to sizeof(T))
//                   /        ___________________
//                  /  /---> |___________________ buffer
//                 /  /       ___________________
//                /  /  /--> |___________________ buffer
//               /  /  /
//              /  /  /  /-> ...
//            _/__/__/__/___
// map_ptr-> |__|__|__|__|__ ...
//            ^
//            |
//           node
//

template <typename T>
struct DequeConstIterator {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using MapPtr = T**;
    using Self = DequeConstIterator<T>;

    constexpr static size_type buffer_size() {
        constexpr size_type sz = sizeof(T);
        return sz < 512 ? static_cast<size_type>(512 / sz)
                        : static_cast<size_type>(1);
    }

    T* cur;      // point to current element
    T* first;    // point to first element in current buffer
    T* last;     // point to last element in current buffer
    MapPtr node; // point to the control center

    DequeConstIterator() = default;
    DequeConstIterator(T* c, T* f, T* l, MapPtr n)
        : cur(c), first(f), last(l), node(n) {
    }
    DequeConstIterator(const Self&) = default;

    DequeConstIterator(DequeConstIterator&& rhs) noexcept
        : cur(rhs.cur), first(rhs.first), last(rhs.last), node(rhs.node) {
        rhs.cur = nullptr;
        rhs.first = nullptr;
        rhs.last = nullptr;
        rhs.node = nullptr;
    }

    DequeConstIterator& operator=(DequeConstIterator&& rhs) noexcept = default;

    DequeConstIterator& operator=(const DequeConstIterator&) = default;

    void setNode(MapPtr new_node) {
        node = new_node;
        first = *new_node;
        last = first + static_cast<difference_type>(buffer_size());
    }

    reference operator*() const {
        return *cur;
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    Self& operator++() {
        ++cur;
        if (cur == last) {     // current point to the tail of buffer
            setNode(node + 1); // switch to the next buffer
            cur = first;       // current point to the first element
        }
        return *this;
    }

    Self operator++(int) {
        Self tmp;
        ++*this;
        return tmp;
    }

    Self& operator--() {
        if (cur == first) {
            setNode(node - 1);
            cur = last;
        }
        --cur;
        return *this;
    }

    Self operator--(int) {
        Self tmp;
        --*this;
        return tmp;
    }

    Self& operator+=(difference_type n) {
        difference_type offset = n + (cur - first);
        if (offset >= 0 &&
            offset < static_cast<difference_type>(buffer_size())) {
            // the target is in the same buffer
            cur += n;
        } else {
            // the target isn't in the same buffer
            difference_type node_offset =
                offset > 0
                    ? offset / static_cast<difference_type>(buffer_size())
                    : -static_cast<difference_type>((-offset - 1) /
                                                    buffer_size()) -
                          1;

            setNode(node + node_offset);
            cur = first + (offset - node_offset * static_cast<difference_type>(
                                                      buffer_size()));
        }

        return *this;
    }

    Self operator+(difference_type n) const {
        Self tmp = *this;
        tmp += n;

        return tmp;
    }

    Self& operator-=(difference_type n) {
        return *this += (-n);
    }

    Self operator-(difference_type n) const {
        Self tmp = *this;
        tmp -= n;

        return tmp;
    }

    difference_type operator-(const Self& rhs) const {
        return static_cast<difference_type>(buffer_size()) *
                   (node - rhs.node - 1) +
               (cur - first) + (rhs.last - rhs.cur);
    }

    reference operator[](difference_type n) const {
        return *(*this + n);
    }

    bool operator==(const Self& rhs) const {
        return cur == rhs.cur;
    }

    bool operator!=(const Self& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(const Self& rhs) const {
        return node == rhs.node ? cur < rhs.cur : node < rhs.node;
    }

    bool operator>(const Self& rhs) const {
        return rhs < *this;
    }

    bool operator<=(const Self& rhs) const {
        return !(rhs < *this);
    }

    bool operator>=(const Self& rhs) const {
        return !(*this < rhs);
    }
}; // class DequeConstIterator<T>

template <typename T>
inline DequeConstIterator<T>
operator+(typename DequeConstIterator<T>::difference_type n,
          DequeConstIterator<T> iter) {
    return iter += n;
}

template <typename T>
struct DequeIterator : DequeConstIterator<T> {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using MapPtr = T**;
    using Base = DequeConstIterator<T>;
    using Self = DequeIterator<T>;

    using Base::cur;

    DequeIterator() : Base() {
    }
    DequeIterator(T* c, T* f, T* l, MapPtr n) : Base(c, f, l, n) {
    }
    DequeIterator(const Self&) = default;

    DequeIterator(DequeIterator&& rhs) noexcept
        : Base(rhs.cur, rhs.first, rhs.last, rhs.node) {
        rhs.cur = nullptr;
        rhs.first = nullptr;
        rhs.last = nullptr;
        rhs.node = nullptr;
    }

    DequeIterator& operator=(const DequeIterator&) = default;

    reference operator*() const {
        return *cur;
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    Self& operator++() {
        ++*static_cast<Base*>(this);
        return *this;
    }

    Self operator++(int) {
        Self tmp;
        ++*this;
        return tmp;
    }

    Self& operator--() {
        --*static_cast<Base*>(this);
        return *this;
    }

    Self operator--(int) {
        Self tmp;
        --*this;
        return tmp;
    }

    Self& operator+=(difference_type n) {
        *static_cast<Base*>(this) += n;
        return *this;
    }

    Self operator+(difference_type n) const {
        Self tmp = *this;
        tmp += n;
        return tmp;
    }

    Self& operator-=(difference_type n) {
        return *this += (-n);
    }

    Self operator-(difference_type n) const {
        Self tmp = *this;
        tmp -= n;

        return tmp;
    }

    difference_type operator-(const Self& rhs) const {
        return *static_cast<const Base*>(this) - rhs;
    }

    reference operator[](difference_type n) const {
        return *(*this + n);
    }
}; // class DequeIterator<T>

template <typename T>
inline DequeIterator<T> operator+(typename DequeIterator<T>::difference_type n,
                                  DequeIterator<T> iter) {
    return iter += n;
}

template <typename T, typename Alloc>
class DequeBase {
public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = DequeIterator<T>;
    using const_iterator = DequeConstIterator<T>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

protected:
    using MapPtr = pointer*;
    using AlPtr =
        typename allocator_traits<Alloc>::template rebind_alloc<pointer>;

protected:
    iterator start;
    iterator finish;
    MapPtr map_ptr;
    size_type map_size;
    Alloc alloc;
    AlPtr alloc_map;

    constexpr static const size_type kBufferSize = iterator::buffer_size();

    constexpr static const size_type kSmallestSize = 8;

protected:
    MapPtr allocateMap(size_type n) {
        MapPtr p = nullptr;
        try {
            p = alloc_map.allocate(n);
            return p;
        } catch (...) {
            deallocateMap(p, n);
            throw;
        }
    }

    void deallocateMap(MapPtr m, size_type n) {
        alloc_map.deallocate(m, n);
    }

    T* allocateNode() {
        T* node = nullptr;
        try {
            node = alloc.allocate(kBufferSize);
            return node;
        } catch (...) {
            deallocateNode(node);
            throw;
        }
    }

    void deallocateNode(T* p) {
        alloc.deallocate(p, kBufferSize);
    }

    void initializerMap(size_type n) {
        size_type num_nodes = n / kBufferSize + 1;

        map_size =
            tiny_stl::max(static_cast<size_type>(kSmallestSize), num_nodes + 2);

        MapPtr nStart = nullptr, nFinish = nullptr;
        try {
            map_ptr = allocateMap(map_size);

            nStart = map_ptr + (map_size - num_nodes) / 2; // centre position
            nFinish = nStart + num_nodes;

            createNodes(nStart, nFinish); // allocate buffer
        } catch (...) {
            deallocateMap(nStart, nFinish - nStart);
            throw;
        }

        start.setNode(nStart); // set iterator
        start.cur = start.first;

        finish.setNode(nFinish - 1);
        finish.cur = finish.first + n % kBufferSize;
    }

    void createNodes(MapPtr nstart, MapPtr nfinish) {
        for (MapPtr cur = nstart; cur != nfinish; ++cur)
            *cur = allocateNode();
    }

    void deallocNodes(MapPtr nstart, MapPtr nfinish) {
        for (MapPtr cur = nstart; cur != nfinish; ++cur)
            if (cur != nullptr)
                deallocateNode(*cur);
    }

public:
    DequeBase(const Alloc& a)
        : start(), finish(), map_ptr(), map_size(0), alloc(a), alloc_map() {
    }

    DequeBase(const Alloc& a, size_type num_elements)
        : start(), finish(), map_ptr(), map_size(0), alloc(a), alloc_map() {
        initializerMap(num_elements);
    }

    ~DequeBase() {
        if (map_ptr != nullptr) {
            deallocNodes(start.node, finish.node + 1);
            deallocateMap(map_ptr, map_size);
        }
    }
}; // class DequeBase<T, Alloc>

template <typename T, typename Alloc = allocator<T>>
class deque : public DequeBase<T, Alloc> {
public:
    static_assert(is_same<T, typename Alloc::value_type>::value,
                  "Allocator::value_type is not the same as T");

public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = DequeIterator<T>;
    using const_iterator = DequeConstIterator<T>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

protected:
    using MapPtr = pointer*;
    using AlPtr =
        typename allocator_traits<Alloc>::template rebind_alloc<pointer>;
    using Base = DequeBase<T, Alloc>;
    using Self = deque<T, Alloc>;

private:
    using Base::alloc;
    using Base::alloc_map;
    using Base::allocateNode;
    using Base::finish;
    using Base::kBufferSize;
    using Base::map_ptr;
    using Base::map_size;
    using Base::start;

private:
    void fillInitialize(const T& val) {
        for (MapPtr node = this->start.node; node != this->finish.node; ++node)
            // fill complete buffer
            uninitialized_fill(*node, *node + kBufferSize, val);
        uninitialized_fill(this->finish.first, this->finish.cur, val);
    }

    void tidy() {
        destroy(this->start, this->finish);
    }

public:
    // (1)
    deque() : deque(Alloc()) {
    }
    explicit deque(const Alloc& a) : Base(a, 0) {
    }

    // (2)
    deque(size_type count, const T& val, const Alloc& a = Alloc())
        : Base(a, count) {
        fillInitialize(val);
    }

    // (3)
    explicit deque(size_type count, const Alloc& a = Alloc()) : Base(a, count) {
        fillInitialize(T{});
    }

    // (4)
    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    deque(InIter first, InIter last, const Alloc& a = Alloc())
        : Base(a, last - first) {
        try {
            uninitialized_copy(first, last, this->start);
        } catch (...) {
            tidy();
            throw;
        }
    }

    // (5)
    deque(const deque& rhs)
        : Base(allocator_traits<Alloc>::select_on_container_copy_construction(
                   rhs.alloc),
               rhs.size()) {
        try {
            uninitialized_copy(rhs.begin(), rhs.end(), this->start);
        } catch (...) {
            tidy();
            throw;
        }
    }

    // (5)
    deque(const deque& rhs, const Alloc& a) : Base(a, rhs.size()) {
        try {
            uninitialized_copy(rhs.begin(), rhs.end(), this->start);
        } catch (...) {
            tidy();
            throw;
        }
    }

private:
    void assignMove(deque&& rhs, true_type) {
        this->map_ptr = rhs.map_ptr;
        this->map_size = rhs.map_size;
        this->start = tiny_stl::move(rhs.start);
        this->finish = tiny_stl::move(rhs.finish);

        rhs.map_ptr = nullptr;
        rhs.map_size = 0;
        rhs.start = iterator();
        rhs.finish = iterator();
    }

    template <typename Iter>
    void constructHelper(Iter first, Iter last) {
        while (first != last)
            emplace_back(*first);
    }

    void assignMove(deque&& rhs, false_type) {
        if (this->alloc == rhs.alloc)
            assignMove(tiny_stl::move(rhs), true_type{});

        else
            constructHelper(tiny_stl::make_move_iterator(begin()),
                            tiny_stl::make_move_iterator(end()));
    }

public:
    // (6)
    deque(deque&& rhs) noexcept : Base(tiny_stl::move(rhs.alloc)) {
        assignMove(tiny_stl::move(rhs), true_type{});
    }

    // (7)
    deque(deque&& rhs, const Alloc& a) : Base(a) {
        assignMove(tiny_stl::move(rhs), false_type{});
    }

    // (8)
    deque(std::initializer_list<T> ilist, const Alloc& a = Alloc())
        : Base(a, ilist.size()) {
        try {
            uninitialized_copy(ilist.begin(), ilist.end(), this->start);
        } catch (...) {
            tidy();
            throw;
        }
    }

    ~deque() {
        tidy();
    }

    deque& operator=(const deque& rhs) {
        assert(this != tiny_stl::addressof(rhs));

        if (this->alloc != rhs.alloc)
            tidy();

        if (allocator_traits<
                Alloc>::propagate_on_container_copy_assignment::value) {
            this->alloc = rhs.alloc;
            this->alloc_map = rhs.alloc_map;
        }

        try {
            if (this->size() >= rhs.size()) { // too many
                iterator mid = tiny_stl::copy(rhs.begin(), rhs.end(), begin());
                erase(mid, end());
            } else { // too little
                const_iterator mid = rhs.begin() + this->size();
                tiny_stl::copy(rhs.begin(), mid, begin());
                insert(end(), mid, rhs.end());
            }
        } catch (...) {
            tidy();
            throw;
        }

        return *this;
    }

    deque& operator=(deque&& rhs) noexcept(
        noexcept(allocator_traits<Alloc>::is_always_equal::value)) {
        assert(this != tiny_stl::addressof(rhs));
        tidy();
        this->alloc = tiny_stl::move(rhs.alloc);
        this->alloc_map = tiny_stl::move(rhs.alloc_map);
        assignMove(tiny_stl::move(rhs),
                   typename allocator_traits<
                       Alloc>::propagate_on_container_move_assignment{});

        return *this;
    }

    deque& operator=(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void assign(size_type count, const T& val) {
        clear();
        insertN(begin(), count, val);
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    void assign(InIter first, InIter last) {
        clear();
        for (; first != last; ++first)
            emplace_back(*first);
    }

    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

public:
    reference at(size_type pos) {
        assert(pos < this->size());
        return start[static_cast<difference_type>(pos)];
    }

    const_reference at(size_type pos) const {
        assert(pos < this->size());
        return start[static_cast<difference_type>(pos)];
    }

    reference operator[](size_type pos) {
        return start[static_cast<difference_type>(pos)];
    }

    const_reference operator[](size_type pos) const {
        return start[static_cast<difference_type>(pos)];
    }

    reference front() {
        assert(!empty());
        return *start;
    }

    const_reference front() const {
        assert(!empty());
        return *start;
    }

    reference back() {
        assert(!empty());
        return *(finish - 1);
    }

    const_reference back() const {
        assert(!empty());
        return *(finish - 1);
    }

    allocator_type get_allocator() const {
        return this->alloc;
    }

public:
    iterator begin() noexcept {
        return start;
    }

    const_iterator begin() const noexcept {
        return const_iterator(start.cur, start.first, start.last, start.node);
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return finish;
    }

    const_iterator end() const noexcept {
        return const_iterator(finish.cur, finish.first, finish.last,
                              finish.node);
    }

    const_iterator cend() const noexcept {
        return end();
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(begin());
    }

public:
    bool empty() const noexcept {
        return start == finish;
    }

    size_type size() const noexcept {
        return finish - start;
    }

    size_type max_size() const noexcept {
        return static_cast<size_type>(-1);
    }

    void shrink_to_fit() {
        deque tmp{tiny_stl::make_move_iterator(begin()),
                  tiny_stl::make_move_iterator(end())};
        swap(tmp);
    }

public:
    void clear() noexcept {
        // Except for the first and last buffers
        for (MapPtr p = start.node + 1; p < finish.node; ++p) {
            destroy(*p, *p + kBufferSize);
            this->alloc.deallocate(*p, kBufferSize);
        }

        // There are at least two buffers
        if (start.node != finish.node) {
            destroy(start.cur, start.last);
            destroy(finish.first, finish.cur);

            // Release the last buffer, reserve the first buffer
            this->alloc.deallocate(finish.first, kBufferSize);
        } else {
            // There is only one buffer, reserve the buffer, no deallocate
            destroy(start.cur, finish.cur);
        }

        finish = start;
    }

private:
    // change the map_ptr at the front
    void reallocateMap(size_type num_add, true_type) {
        size_type old_num_nodes = finish.node - start.node + 1;
        size_type new_num_nodes = old_num_nodes + num_add;

        MapPtr new_nstart;

        if (map_size > 2 * new_num_nodes) { // no reallocate
            new_nstart = map_ptr + (map_size - new_num_nodes) / 2 + num_add;

            // Avoid coverage
            try {
                if (new_nstart < start.node)
                    copy(start.node, finish.node + 1, new_nstart);
                else
                    copy_backward(start.node, finish.node + 1,
                                  new_nstart + old_num_nodes);
            } catch (...) {
                tidy();
                throw;
            }
        } else { // reallocate
            size_type new_map_size =
                map_size + tiny_stl::max(map_size, num_add) + 2;

            MapPtr new_map = nullptr;
            try {
                new_map = this->alloc_map.allocate(new_map_size);
                new_nstart =
                    new_map + (new_map_size - new_num_nodes) / 2 + num_add;

                copy(start.node, finish.node + 1,
                     new_nstart); // copy origin node to new map
            } catch (...) {

                tidy();
                throw;
            }

            this->alloc_map.deallocate(map_ptr, map_size);

            map_ptr = new_map;
            map_size = new_map_size;
        }

        // reset the iterators
        start.setNode(new_nstart);
        finish.setNode(new_nstart + old_num_nodes - 1);
    }

    // change the map_ptr at the front
    void reallocateMap(size_type num_add, false_type) {
        size_type old_num_nodes = finish.node - start.node + 1;
        size_type new_num_nodes = old_num_nodes + num_add;

        size_type new_map_size =
            map_size + tiny_stl::max(map_size, num_add) + 2;

        MapPtr new_map = nullptr, new_nstart = nullptr;
        try {
            new_map = this->alloc_map.allocate(new_map_size); // reallocate
            new_nstart = new_map + (new_map_size - new_num_nodes) / 2;

            copy(start.node, finish.node + 1,
                 new_nstart); // copy origin node to new map
        } catch (...) {
            this->alloc_map.deallocate(new_map, new_map_size);
            throw;
        }

        this->alloc_map.deallocate(map_ptr, map_size);

        map_ptr = new_map;
        map_size = new_map_size;

        start.setNode(new_nstart);
        finish.setNode(new_nstart + old_num_nodes - 1);
    }

    void reserveMapAtFront(size_type num_add) {
        // change the map_ptr
        if (static_cast<difference_type>(num_add) > start.node - map_ptr)
            reallocateMap(num_add, true_type{});
    }

    void reserveMapAtBack(size_type num_add) {
        // change the map_ptr
        if (num_add + 1 + (finish.node - map_ptr) > map_size)
            reallocateMap(num_add, false_type{});
    }

    template <typename... Args>
    void emplaceFrontAux(Args&&... args) {
        reserveMapAtFront(1);

        try {
            // allocate a new node
            *(start.node - 1) = allocateNode();

            start.setNode(start.node - 1);
            start.cur = start.last - 1;
            this->alloc.construct(start.cur, tiny_stl::forward<Args>(args)...);
        } catch (...) {
        }
    }

    template <typename... Args>
    void emplaceBackAux(Args&&... args) {
        reserveMapAtBack(1);

        try {
            // allocate a new node
            *(finish.node + 1) = allocateNode();
            this->alloc.construct(finish.cur, tiny_stl::forward<Args>(args)...);
        } catch (...) {
            tidy();
            throw;
        }

        finish.setNode(finish.node + 1);
        finish.cur = finish.first;
    }

public:
    template <typename... Args>
    void emplace_back(Args&&... args) {
        assert(size() < max_size() - 1);
        try {
            if (finish.cur != finish.last - 1) { // There are two or more spaces
                alloc.construct(finish.cur, tiny_stl::forward<Args>(args)...);
                ++finish.cur;
            } else { // There is only one space
                emplaceBackAux(tiny_stl::forward<Args>(args)...);
            }
        } catch (...) {
            tidy();
            throw;
        }
    }

    void push_back(const T& val) {
        emplace_back(val);
    }

    void push_back(T&& val) {
        emplace_back(tiny_stl::move(val));
    }

    template <typename... Args>
    void emplace_front(Args&&... args) {
        assert(size() < max_size() - 1);
        if (start.cur != start.first) {
            this->alloc.construct(start.cur - 1,
                                  tiny_stl::forward<Args>(args)...);
            --start.cur;
        } else
            emplaceFrontAux(tiny_stl::forward<Args>(args)...);
    }

    void push_front(const T& val) {
        emplace_front(val);
    }

    void push_front(T&& val) {
        emplace_front(tiny_stl::move(val));
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        iterator iter = makeIter(pos);
        assert(iter >= begin() && iter <= end());
        size_type offset = iter - begin();

        if (offset <= size() / 2) { // front
            emplace_front(tiny_stl::forward<Args>(args)...);
            tiny_stl::rotate(begin(), begin() + 1, begin() + 1 + offset);
        } else { // back
            emplace_back(tiny_stl::forward<Args>(args)...);
            tiny_stl::rotate(begin() + offset, end() - 1, end());
        }

        return begin() + offset;
    }

    iterator insert(const_iterator pos, const T& val) {
        return emplace(pos, val);
    }

    iterator insert(const_iterator pos, T&& val) {
        return emplace(pos, tiny_stl::move(val));
    }

private:
    void insertN(const_iterator pos, size_type count, const T& val) {
        iterator iter = makeIter(pos);
        assert(iter >= begin() && iter <= end());
        size_type offset = iter - begin();
        size_type reoffset = size() - offset;
        size_type oldSize = size();

        if (offset < oldSize / 2) { // front
            if (offset < count) {   // prefix < count
                for (size_type i = count - offset; i > 0; --i)
                    push_front(val); // insert excess val
                for (size_type i = offset; i > 0; --i)
                    push_front(begin()[count - 1]); // insert prefix
                iterator mid = begin() + count;
                tiny_stl::fill(mid, mid + offset, val);
            } else { // prefix >= count
                for (size_type i = count; i > 0; --i)
                    push_front(begin()[count - 1]); // push part of prefix

                iterator mid = begin() + count;
                tiny_stl::move(mid + count, mid + offset, mid);
                tiny_stl::fill(begin() + offset, mid + offset, val);
            }
        } else {                    // back
            if (reoffset < count) { // suffix < count
                for (size_type i = count - reoffset; i > 0; --i)
                    push_back(val); // insert excess val
                for (size_type i = 0; i < reoffset; ++i)
                    push_back(begin()[offset + i]);
                iterator mid = begin() + offset;
                tiny_stl::fill(mid, mid + reoffset, val);
            } else { // suffix >= count
                for (size_type i = 0; i < count; ++i)
                    push_back(
                        begin()[oldSize - count + i]); // push part of suffix
                iterator mid = begin() + offset;
                tiny_stl::move_backward(mid, mid + reoffset - count,
                                        mid + reoffset);
                tiny_stl::fill(mid, mid + count, val);
            }
        }
    }

public:
    iterator insert(const_iterator pos, size_type count, const T& val) {
        size_type offset = makeIter(pos) - begin();
        insertN(pos, count, val);
        return begin() + offset;
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    iterator insert(const_iterator pos, InIter first, InIter last) {
        iterator iter = makeIter(pos);
        assert(iter >= begin() && iter <= end());
        size_type offset = iter - begin();
        size_type oldSize = size();

        if (last <= first) {
            // do nothing
        } else if (offset <= oldSize / 2) { // front
            for (; first != last; ++first)
                push_front(*first);

            size_type num = size() - oldSize;
            tiny_stl::reverse(begin(), begin() + num);
            tiny_stl::rotate(begin(), begin() + num, begin() + num + offset);
        } else { // back
            for (; first != last; ++first)
                push_back(*first);
            tiny_stl::rotate(begin() + offset, begin() + oldSize, end());
        }

        return begin() + offset;
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

private:
    void popBackAux() {
        this->deallocateNode(finish.first); // release the last buffer
        finish.setNode(finish.node - 1);
        finish.cur = finish.last - 1;
        this->alloc.destroy(finish.cur);
    }

    void popFrontAux() {
        this->alloc.destroy(start.cur);
        this->deallocateNode(start.first);
        start.setNode(start.node + 1);
        start.cur = start.first;
    }

public:
    void pop_back() {
        assert(!empty());
        if (finish.cur != finish.first) {
            // Have one or more elements
            --finish.cur;
            this->alloc.destroy(finish.cur);
        } else {
            popBackAux();
        }
    }

    void pop_front() {
        assert(!empty());
        if (start.cur != start.last - 1) {
            this->alloc.destroy(start.cur);
            ++start.cur;
        } else {
            popFrontAux();
        }
    }

private:
    iterator makeIter(const_iterator citer) const {
        return iterator(citer.cur, citer.first, citer.last, citer.node);
    }

public:
    iterator erase(const_iterator pos) {
        assert(makeIter(pos) != end());
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last) {
        iterator f = makeIter(first);
        iterator l = makeIter(last);

        assert(f == l || (f < l && f >= begin() && l <= end()));

        if (f == l && last == finish) {
            clear();
            return end();
        } else {
            size_type num_erase = l - f;
            size_type num_before = f - start;

            if (num_before < (size() - num_erase) / 2) { // front
                move_backward(start, f, l);
                iterator new_start = start + num_erase;
                destroy(start, new_start);

                for (MapPtr cur = start.node; cur < new_start.node; ++cur)
                    this->deallocateNode(*cur);
                start = new_start;
            } else { // back
                tiny_stl::move(l, finish, f);
                iterator new_finish = finish - num_erase;
                destroy(new_finish, finish);

                for (MapPtr cur = new_finish.node + 1; cur <= finish.node;
                     ++cur)
                    this->deallocateNode(*cur);

                finish = new_finish;
            }

            return start + num_before;
        }
    }

    void resize(size_type count) {
        while (this->size() < count) // this too little
            emplace_back();
        while (this->size() > count)
            pop_back();
    }

    void resize(size_type count, const T& val) {
        while (this->size() < count) // this too little
            emplace_back(val);
        while (this->size() > count)
            pop_back();
    }

    void swap(deque& rhs) noexcept(
        noexcept(allocator_traits<Alloc>::is_always_equal::value)) {
        swapAlloc(this->alloc, rhs.alloc);
        swapAlloc(this->alloc_map, rhs.alloc_map);

        tiny_stl::swap(this->map_ptr, rhs.map_ptr);
        tiny_stl::swap(this->start, rhs.start);
        tiny_stl::swap(this->finish, rhs.finish);
        tiny_stl::swap(this->map_size, rhs.map_size);
    }

}; // class deque<T, Alloc>

template <typename T, typename Alloc>
inline bool operator==(const deque<T, Alloc>& lhs, const deque<T, Alloc>& rhs) {
    return lhs.size() == rhs.size() &&
           tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Alloc>
inline bool operator!=(const deque<T, Alloc>& lhs, const deque<T, Alloc>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Alloc>
inline bool operator<(const deque<T, Alloc>& lhs, const deque<T, Alloc>& rhs) {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename T, typename Alloc>
inline bool operator>(const deque<T, Alloc>& lhs, const deque<T, Alloc>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Alloc>
inline bool operator<=(const deque<T, Alloc>& lhs, const deque<T, Alloc>& rhs) {
    return !(rhs < lhs);
}

template <typename T, typename Alloc>
inline bool operator>=(const deque<T, Alloc>& lhs, const deque<T, Alloc>& rhs) {
    return !(lhs < rhs);
}

template <typename T, typename Alloc>
inline void swap(deque<T, Alloc>& lhs,
                 deque<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace tiny_stl
