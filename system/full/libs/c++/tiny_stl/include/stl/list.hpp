// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <initializer_list>

#include "memory.hpp"

namespace tiny_stl {

template <typename T>
struct LNode {
    T data;

    LNode<T>* prev;
    LNode<T>* next;
};

template <typename T>
struct ListConstIterator {
    using iterator_category = bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;
    using Ptr = LNode<T>*;

    Ptr ptr = nullptr;

    ListConstIterator() = default;
    ListConstIterator(Ptr x) : ptr(x) {
    }
    ListConstIterator(const ListConstIterator& rhs) = default;
    ListConstIterator& operator=(const ListConstIterator& rhs) = default;

    reference operator*() const {
        return ptr->data;
    }

    pointer operator->() const {
        // &ptr->data, avoid overloading operator&
        return pointer_traits<pointer>::pointer_to(**this);
    }

    ListConstIterator& operator++() {
        ptr = ptr->next;
        return *this;
    }

    ListConstIterator& operator--() {
        ptr = ptr->prev;
        return *this;
    }

    ListConstIterator operator++(int) {
        ListConstIterator tmp = *this;
        ptr = ptr->next;
        return tmp;
    }

    ListConstIterator operator--(int) {
        ListConstIterator tmp = *this;
        ptr = ptr->prev;
        return tmp;
    }

    bool operator==(const ListConstIterator& rhs) const {
        return ptr == rhs.ptr;
    }

    bool operator!=(const ListConstIterator& rhs) const {
        return ptr != rhs.ptr;
    }
}; // class ListConstIterator<T>

template <typename T>
struct ListIterator : ListConstIterator<T> {
    using iterator_category = bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    using Ptr = LNode<T>*;
    using Base = ListConstIterator<T>;

    ListIterator() : Base() {
    }
    ListIterator(Ptr x) : Base(x) {
    }
    ListIterator(const ListIterator& rhs) : Base(rhs.ptr) {
    }

    reference operator*() const {
        return this->ptr->data;
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    ListIterator& operator++() {
        this->ptr = this->ptr->next;
        return *this;
    }

    ListIterator& operator--() {
        this->ptr = this->ptr->prev;
        return *this;
    }

    ListIterator operator++(int) {
        ListIterator tmp = *this;
        Base::ptr = Base::ptr->next;
        return tmp;
    }

    ListIterator operator--(int) {
        ListIterator tmp = *this;
        Base::ptr = Base::ptr->prev;
        return tmp;
    }
}; // class ListIterator

template <typename T, typename Alloc>
class ListBase {
public:
    using value_type = T;
    using allocator_type = Alloc; // In face, will be not be used
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename allocator_traits<Alloc>::pointer;
    using const_pointer = typename allocator_traits<Alloc>::const_pointer;
    using iterator = ListIterator<T>;
    using const_iterator = ListConstIterator<T>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

    using Node = LNode<T>;
    using NodePtr = LNode<T>*;
    using AlNode =
        typename allocator_traits<Alloc>::template rebind_alloc<Node>;

protected:
    NodePtr head;
    size_type count;
    AlNode alloc;

    ListBase() : count(0) {
        constructHeadNode();
    }

    template <
        typename Any_alloc,
        typename = enable_if_t<!is_same<decay_t<Any_alloc>, ListBase>::value>>
    ListBase(Any_alloc anyAlloc) : count(0), alloc(anyAlloc) {
        constructHeadNode();
    }

    void constructHeadNode() {
        try {
            head = alloc.allocate(1);
            head->prev = head;
            head->next = head;
        } catch (...) {
            freeHeadNode();
            throw;
        }
    }

    void freeHeadNode() {
        alloc.deallocate(head, 1);
    }

    ~ListBase() {
        freeHeadNode();
    }
}; // class ListBase

// cycle linked list
//   _______________________________________________
//  |                                               |
//  |   head(end)    begin                          |
//  |    ______     ______     ______     ______    |
//  |   | null |   | data |   | data |   | data |   |
//  ----| prev |<--| prev |<--| prev |<--| prev |<---
//  --->|_next_|-->|_next_|-->|_next_|-->|_next_|----
//  |                                               |
//  |_______________________________________________|
//

template <typename T, typename Alloc = allocator<T>>
class list : public ListBase<T, Alloc> {
public:
    using value_type = T;
    using allocator_type = Alloc; // In fact, will be not be used
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename allocator_traits<Alloc>::pointer;
    using const_pointer = typename allocator_traits<Alloc>::const_pointer;
    using iterator = ListIterator<T>;
    using const_iterator = ListConstIterator<T>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

    using Base = ListBase<T, Alloc>;
    using Node = LNode<T>;
    using NodePtr = LNode<T>*;
    using AlNode =
        typename allocator_traits<Alloc>::template rebind_alloc<Node>;
    using AlNodeTraits = allocator_traits<AlNode>;

private:
    template <typename... Args>
    NodePtr allocAndConstruct(Args&&... args) {
        assert(Base::count < max_size() - 1);

        NodePtr p = nullptr;

        try {
            p = this->alloc.allocate(1);
            AlNodeTraits::construct(this->alloc, tiny_stl::addressof(p->data),
                                    tiny_stl::forward<Args>(args)...);
        } catch (...) {
            this->alloc.deallocate(p, 1);
            throw;
        }

        return p;
    }

    void destroyAndFree(NodePtr p) {
        this->alloc.destroy(tiny_stl::addressof(p->data));
        this->alloc.deallocate(p, 1);
    }

    void constructN(size_type n, const T& val) {
        try {
            insertN(begin(), n, val);
        } catch (...) {
            clear();
            throw;
        }
    }

    template <typename InIter>
    void constructRange(InIter first, InIter last) {
        try {
            insert(begin(), first, last);
        } catch (...) {
            clear();
            throw;
        }
    }

    void constructMove(list&& rhs, true_type) {
        tiny_stl::swap(this->head, rhs.head);
        tiny_stl::swap(Base::count, rhs.count);
    }

    void constructMove(list&& rhs, false_type) {
        if (this->alloc == rhs.alloc)
            constructMove(tiny_stl::move(rhs), true_type{});
        else
            constructRange(tiny_stl::make_move_iterator(rhs.begin()),
                           tiny_stl::make_move_iterator(rhs.end()));
    }

public:
    // (1)
    list() : list(Alloc()) {
    }
    explicit list(const Alloc& a) : Base(a) {
    }

    // (2)
    list(size_type n, const T& val, const Alloc& a = Alloc()) : Base(a) {
        constructN(n, val);
    }

    // (3)
    explicit list(size_type n, const Alloc& a = Alloc()) : Base(a) {
        resize(n);
    }

    // (4)
    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    list(InIter first, InIter last, const Alloc& a = Alloc()) : Base(a) {
        constructRange(first, last);
    }

    // (5)
    list(const list& rhs)
        : Base(AlNodeTraits::select_on_container_copy_construction(rhs.alloc)) {
        try {
            insert(begin(), rhs.begin(), rhs.end());
        } catch (...) {
            clear();
            throw;
        }
    }

    // (5)
    list(const list& rhs, const Alloc& a) : Base(a) {
        try {
            insert(begin(), rhs.begin(), rhs.end());
        } catch (...) {
            clear();
            throw;
        }
    }

    // (6)
    list(list&& rhs) noexcept : Base(tiny_stl::move(rhs.alloc)) {
        constructMove(tiny_stl::move(rhs), true_type{});
    }

    // (7)
    list(list&& rhs, const Alloc& a) : Base(a) {
        constructMove(tiny_stl::move(rhs), false_type{});
    }

    // (8)
    list(std::initializer_list<T> ilist, const Alloc& a = Alloc()) : Base(a) {
        try {
            insert(begin(), ilist.begin(), ilist.end());
        } catch (...) {
            clear();
            throw;
        }
    }

    template <typename Arg>
    void reuseNode(iterator pos, Arg&& arg) {
        AlNodeTraits::destroy(this->alloc, tiny_stl::addressof(pos.ptr->data));

        try {
            AlNodeTraits::construct(this->alloc,
                                    tiny_stl::addressof(pos.ptr->data),
                                    tiny_stl::forward<Arg>(arg));
        } catch (...) {
            // FIXME, only unlink the node
            clear();
            throw;
        }
    }

    void assign(size_type n, const T& val) {
        iterator old = begin();
        size_type i;

        try {
            for (i = 0; i < n && old != end(); ++i, ++old)
                reuseNode(old, val); // no alloc, reuse nodes
            for (; i < n; ++i)
                insertAux(end(), val); // last - first > count, alloc
        } catch (...) {
            clear();
            throw;
        }

        erase(old, end()); // last - first < count, erase
    }

    template <typename Iter, typename = enable_if_t<is_iterator<Iter>::value>>
    void assign(Iter first, Iter last) {
        iterator old = begin();

        try {
            for (; first != last && old != end(); ++first, ++old)
                reuseNode(old, *first); // no alloc, reuse nodes
            for (; first != last; ++first)
                insertAux(end(), *first); // last - first > count, alloc
        } catch (...) {
            clear();
            throw;
        }

        erase(old, end()); // last - first < count, erase
    }

    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    list& operator=(const list& rhs) {
        assert(this != tiny_stl::addressof(rhs));
        if (this->alloc != rhs.alloc)
            clear();
        if (AlNodeTraits::propagate_on_container_copy_assignment::value)
            this->alloc = rhs.alloc;
        assign(rhs.begin(), rhs.end());

        return *this;
    }

    list& operator=(list&& rhs) noexcept {
        assert(this != tiny_stl::addressof(rhs));
        clear();
        if (AlNodeTraits::propagate_on_container_move_assignment::value)
            this->alloc = rhs.alloc;

        constructMove(
            tiny_stl::move(rhs),
            typename AlNodeTraits::propagate_on_container_move_assignment{});
        return *this;
    }

    list& operator=(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    allocator_type get_allocator() const {
        return this->alloc;
    }

    ~list() noexcept {
        clear();
    }

public:
    T& front() {
        assert(!this->empty());
        return *begin();
    }

    const T& front() const {
        assert(!this->empty());
        return *begin();
    }

    T& back() {
        assert(!this->empty());
        return *(--end());
    }

    const T& back() const {
        assert(!this->empty());
        return *(--end());
    }

    iterator begin() noexcept {
        return iterator(this->head->next);
    }

    const_iterator begin() const noexcept {
        return const_iterator(this->head->next);
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return iterator(this->head);
    }

    const_iterator end() const noexcept {
        return const_iterator(this->head);
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
        return rbegin();
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

    bool empty() const noexcept {
        return this->head->next == this->head;
    }

    size_type size() const noexcept {
        return Base::count;
    }

    size_type max_size() const noexcept {
        return this->alloc.max_size();
    }

    void clear() noexcept {
        NodePtr p = this->head->next;
        this->head->next = this->head;
        this->head->prev = this->head;
        Base::count = 0;

        for (NodePtr pNext = p->next; p != this->head;
             p = pNext, pNext = p->next)
            destroyAndFree(p);
    }

private:
    void increaseCount(size_type n) {
#ifndef NDEBUG // DEBUG
        if (Base::count > max_size() - n - 1)
            xLength();
#endif // !NDEBUG
        Base::count += n;
    }

    template <typename... Args>
    void insertAux(const_iterator pos, Args&&... args) {
        NodePtr newNode = allocAndConstruct(tiny_stl::forward<Args>(args)...);
        const NodePtr nextNode = pos.ptr;
        const NodePtr prevNode = nextNode->prev;
        newNode->prev = prevNode;
        newNode->next = nextNode;
        increaseCount(1);
        nextNode->prev = newNode;
        prevNode->next = newNode;
    }

    void insertN(const_iterator pos, size_type n, const T& val) {
        for (; n > 0; --n)
            insertAux(pos, val);
    }

    template <typename InIter>
    void insertRangeAux(const_iterator pos, InIter first, InIter last) {
        for (; first != last; ++first)
            insertAux(pos, *first);
    }

    iterator makeIter(const_iterator pos) const {
        return iterator(pos.ptr);
    }

public:
    // (1)
    iterator insert(const_iterator pos, const T& val) {
        insertAux(pos, val);

        return makeIter(--pos);
    }

    // (2)
    iterator insert(const_iterator pos, T&& val) {
        return emplace(pos, tiny_stl::move(val));
    }

    // (3)
    iterator insert(const_iterator pos, size_type n, const T& val) {
        iterator p = makeIter(pos);
        if (p == begin()) {
            insertN(pos, n, val);
            return begin();
        } else {
            --p;
            insertN(pos, n, val);
            return ++p;
        }
    }

    // (4)
    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    iterator insert(const_iterator pos, InIter first, InIter last) {

        iterator p = makeIter(pos);

        if (p == begin()) {
            insertRangeAux(pos, first, last);
            return begin();
        } else {
            --p;
            insertRangeAux(pos, first, last);
            return ++p;
        }
    }

    // (5)
    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        insertAux(pos, tiny_stl::forward<Args>(args)...);

        return makeIter(--pos);
    }

    iterator erase(const_iterator pos) {
        iterator ret(pos.ptr->next);
        pos.ptr->next->prev = pos.ptr->prev;
        pos.ptr->prev->next = pos.ptr->next;
        destroyAndFree(pos.ptr);
        --Base::count;
        return ret;
    }

    iterator erase(const_iterator first, const_iterator last) {
        if (first == begin() && last == end()) {
            clear();
            return end();
        }

        while (first != last) {
            first = erase(first);
        }
        return makeIter(last);
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        insertAux(end(), tiny_stl::forward<Args>(args)...);
    }

    template <typename... Args>
    void emplace_front(Args&&... args) {
        insertAux(begin(), tiny_stl::forward<Args>(args)...);
    }

    void push_back(const T& val) {
        insertAux(end(), val);
    }

    void push_back(T&& val) {
        insertAux(end(), tiny_stl::move(val));
    }

    void push_front(const T& val) {
        insertAux(begin(), val);
    }

    void push_front(T&& val) {
        insertAux(end(), tiny_stl::move(val));
    }

    void pop_back() {
        erase(--end());
    }

    void pop_front() {
        erase(begin());
    }

    void resize(size_type newSize) {
        // add the nodes at the tail
        if (Base::count < newSize) {
            while (Base::count < newSize)
                insertAux(end());
        } else { // remove the nodes at the tail
            while (Base::count > newSize)
                pop_back();
        }
    }

    void resize(size_type newSize, const T& val) {
        // add the nodes at the tail
        if (Base::count < newSize) {
            insertN(end(), newSize - Base::count, val);
        } else { // remove the nodes at the tail
            while (Base::count > newSize)
                pop_back();
        }
    }

    void swap(list& rhs) noexcept(noexcept(
        allocator_traits<allocator_type>::propagate_on_container_swap::value ||
        allocator_traits<allocator_type>::is_always_equal::value)) {
        swapAlloc(this->alloc, rhs.alloc);
        swapADL(this->head, rhs.head);
        tiny_stl::swap(Base::count, rhs.count);
    }

private:
    // move [first, last) to the front of pos
    void transfer(const_iterator pos, const_iterator first,
                  const_iterator last) {
        if (pos != last) {
            iterator prev = pos.ptr->prev;
            prev.ptr->next = first.ptr;
            first.ptr->prev->next = last.ptr;
            last.ptr->prev->next = pos.ptr;
            pos.ptr->prev = last.ptr->prev;
            last.ptr->prev = first.ptr->prev;
            first.ptr->prev = prev.ptr;
        }
    }

    template <typename Cmp>
    void mergeAux(list& rhs, const Cmp& cmp) {
        if (this != tiny_stl::addressof(rhs)) {
            const_iterator first1 = this->begin(), last1 = this->end();
            const_iterator first2 = rhs.begin(), last2 = rhs.end();

            assert(this->alloc == rhs.alloc);
            assert(tiny_stl::is_sorted(this->begin(), this->end()));
            assert(tiny_stl::is_sorted(rhs.begin(), rhs.end()));

            while (first1 != last1 && first2 != last2) {
                if (cmp(*first2, *first1)) { // *first2 cmp *first1
                    // move *first2 to the front of first1
                    const_iterator next = first2;
                    ++next;
                    this->transfer(first1, first2, next);
                    first2 = next;
                } else {
                    ++first1;
                }
            }

            if (first2 != last2)
                this->transfer(last1, first2, last2);
        }

        Base::count += rhs.count;
        rhs.count = 0;
    }

public:
    void merge(list& rhs) {
        mergeAux(rhs, tiny_stl::less<>{});
    }

    void merge(list&& rhs) {
        mergeAux(rhs, tiny_stl::less<>{});
    }

    template <typename Cmp>
    void merge(list& rhs, Cmp cmp) {
        mergeAux(rhs, cmp);
    }

    template <typename Cmp>
    void merge(list&& rhs, Cmp cmp) {
        mergeAux(rhs, cmp);
    }

    void splice(const_iterator pos, list& rhs) {
        // move all elements from other into *this
        assert(this->alloc == rhs.alloc);
        assert(this != tiny_stl::addressof(rhs));
        if (!rhs.empty()) {
            this->transfer(pos, rhs.begin(), rhs.end()); // modify pointer
            Base::count += rhs.count;
            rhs.count = 0;
        }
    }

    void splice(const_iterator pos, list&& rhs) {
        splice(pos, rhs);
    }

    void splice(const_iterator pos, list& rhs, const_iterator iter) {
        // move the element pointed to by it from other into *this
        assert(this->alloc == rhs.alloc);
        assert(this != tiny_stl::addressof(rhs));

        if (!rhs.empty()) {
            const_iterator last = iter;
            ++last;
            this->transfer(pos, iter, last);
            ++Base::count;
            --rhs.count;
        }
    }

    void splice(const_iterator pos, list&& rhs, const_iterator iter) {
        splice(pos, rhs, iter);
    }

    void splice(const_iterator pos, list& rhs, const_iterator first,
                const_iterator last) {
        // move the elements in the range [first, last) from other into *this
        assert(this->alloc == rhs.alloc);
        assert(this != tiny_stl::addressof(rhs));

        difference_type n = tiny_stl::distance(first, last);

        if (!rhs.empty()) {
            this->transfer(pos, first, last);
            Base::count += n;
            rhs.count -= n;
        }
    }

    void splice(const_iterator pos, list&& rhs, const_iterator first,
                const_iterator last) {
        splice(pos, rhs, first, last);
    }

    void remove(const T& val) {
        for (iterator first = begin(); first != end();) {
            if (*first == val) // remove element
                first = erase(first);
            else // ++iterator
                ++first;
        }
    }

    template <typename UnaryPred>
    void remove_if(UnaryPred pred) {
        for (iterator first = begin(); first != end();) {
            if (pred(*first)) // remove element
                first = erase(first);
            else // ++iterator
                ++first;
        }
    }

    void reserve() noexcept {
        const NodePtr head = this->head;
        NodePtr p = head;

        for (;;) {
            const NodePtr next = p->next;
            p->next = p->prev;
            p->prev = next;

            if (next == head)
                break;
            p = next;
        }
    }

    // Removes all *consecutive duplicate* elements from the container.
    void unique() {
        unique(tiny_stl::equal_to<>{});
    }

    template <typename BinPred>
    void unique(BinPred pred) {
        iterator first = begin();
        iterator next = first;

        while (++next != end()) {
            if (pred(*first, *next)) // match, erase it
                erase(next);
            else
                first = next;

            next = first;
        }
    }

private:
    template <typename Cmp>
    iterator sortAux(iterator first, iterator last, Cmp& cmp, size_type size) {
        if (size < 2)
            return first;

        size_type mid_size = size >> 1;

        iterator mid = first;
        tiny_stl::advance(mid, mid_size);

        first = sortAux(first, mid, cmp, mid_size);
        mid = sortAux(mid, last, cmp, size - mid_size);
        iterator ret = first; // return iter of min *iter

        bool is_once = true;

        while (true) {
            if (cmp(*mid, *first)) { // *mid < *first
                if (is_once)
                    ret = mid;
                iterator next = mid;
                ++next;
                this->transfer(first, mid, next);
                mid = next;
                if (mid == last)
                    return ret;
            } else { // *mid >= *first
                ++first;
                if (first == mid)
                    return ret;
            }

            is_once = false;
        }
    }

public:
    void sort() {
        sort(tiny_stl::less<>{});
    }

    template <typename Cmp>
    void sort(Cmp cmp) {
        sortAux(begin(), end(), cmp, Base::count);
    }

private:
    [[noreturn]] static void xLength() {
        throw "list<T> too long";
    }
}; // class list<T, Alloc>

template <typename T, typename Alloc>
inline bool operator==(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return lhs.size() == rhs.size() &&
           tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Alloc>
inline bool operator!=(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Alloc>
inline bool operator<(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename T, typename Alloc>
inline bool operator>(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Alloc>
inline bool operator<=(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return !(rhs < lhs);
}

template <typename T, typename Alloc>
inline bool operator>=(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) {
    return !(lhs < rhs);
}

template <typename T, typename Alloc>
inline void swap(list<T, Alloc>& lhs,
                 list<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace tiny_stl
