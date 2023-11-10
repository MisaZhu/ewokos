// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <initializer_list>

#include "memory.hpp"

namespace tiny_stl {

template <typename T>
struct FLNode {
    T data;
    FLNode<T>* next;

    FLNode() = default;

    FLNode(T val, FLNode<T>* p) : data(val), next(p) {
    }
};

template <typename T>
struct FListConstIterator {
    using iterator_category = forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;
    using Ptr = FLNode<T>*;

    Ptr ptr = nullptr;

    FListConstIterator() = default;
    FListConstIterator(Ptr x) : ptr(x) {
    }
    FListConstIterator(const FListConstIterator& rhs) = default;
    FListConstIterator& operator=(const FListConstIterator& rhs) = default;

    reference operator*() const {
        return ptr->data;
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    FListConstIterator& operator++() {
        ptr = ptr->next;
        return *this;
    }

    FListConstIterator operator++(int) {
        FListConstIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const FListConstIterator& rhs) const {
        return ptr == rhs.ptr;
    }

    bool operator!=(const FListConstIterator& rhs) const {
        return ptr != rhs.ptr;
    }
}; // class FListConstIterator<T>

template <typename T>
struct FListIterator : FListConstIterator<T> {
    using iterator_category = forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    using Ptr = FLNode<T>*;
    using Base = FListConstIterator<T>;

    FListIterator() : Base() {
    }
    FListIterator(Ptr x) : Base(x) {
    }
    FListIterator(const FListIterator& rhs) : Base(rhs.ptr) {
    }

    reference operator*() const {
        return this->ptr->data;
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    FListIterator& operator++() {
        this->ptr = this->ptr->next;
        return *this;
    }

    FListIterator operator++(int) {
        FListIterator tmp = *this;
        ++*this;
        return tmp;
    }
}; // class FListIterator<T>

template <typename T, typename Alloc>
class FListBase {
public:
    using Node = FLNode<T>;
    using NodePtr = FLNode<T>*;
    using AlNode =
        typename allocator_traits<Alloc>::template rebind_alloc<Node>;

protected:
    extra::compress_pair<AlNode, NodePtr> mPair;

    AlNode& getAlloc() noexcept {
        return mPair.get_first();
    }

    const AlNode& getAlloc() const noexcept {
        return mPair.get_first();
    }

    NodePtr& getHead() noexcept {
        return mPair.get_second();
    }

    const NodePtr& getHead() const noexcept {
        return mPair.get_second();
    }

public:
    FListBase() : mPair() {
        createHeadNode();
    }

    template <
        typename Any_alloc,
        typename = enable_if_t<!is_same<decay_t<Any_alloc>, FListBase>::value>>
    FListBase(Any_alloc&& a) : mPair(tiny_stl::forward<Any_alloc>(a)) {
        createHeadNode();
    }

    void createHeadNode() {
        auto& alloc = getAlloc();
        try {
            getHead() = alloc.allocate(1);
            alloc.construct(tiny_stl::addressof(getHead()->data));
            getHead()->next = nullptr;
        } catch (...) {
            alloc.destroy(tiny_stl::addressof(getHead()->data));
            alloc.deallocate(getHead(), 1);
            throw;
        }
    }

    template <typename... U>
    NodePtr createNode(NodePtr nextNode, U&&... val) {
        auto& alloc = getAlloc();
        NodePtr p = nullptr;
        try {
            p = alloc.allocate(1);
            alloc.construct(tiny_stl::addressof(p->data),
                            tiny_stl::forward<U>(val)...);
        } catch (...) {
            freeNode(p);
            throw;
        }

        p->next = nextNode;

        return p;
    }

    void freeNode(NodePtr p) {
        auto& alloc = getAlloc();
        alloc.destroy(tiny_stl::addressof(p->data));
        alloc.deallocate(p, 1);
    }

    ~FListBase() noexcept {
        auto& alloc = getAlloc();

        alloc.destroy(tiny_stl::addressof(getHead()->data));
        alloc.deallocate(getHead(), 1);
    }
};

//
// head(no element)
//   before_begin    begin                  end
//    _______     ________     ________
//   |  data  |   |  data  |   |  data  |
//   |__next__|-->|__next__|-->|__next__|-->null
//

template <typename T, typename Alloc = allocator<T>>
class forward_list : public FListBase<T, Alloc> {
public:
    static_assert(is_same<T, typename Alloc::value_type>::value,
                  "Alloc::value_type is not the same as T");

public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename allocator_traits<Alloc>::pointer;
    using const_pointer = typename allocator_traits<Alloc>::pointer;
    using iterator = FListIterator<T>;
    using const_iterator = FListConstIterator<T>;

    using Base = FListBase<T, Alloc>;
    using Self = forward_list<T, Alloc>;
    using Node = FLNode<T>;
    using NodePtr = FLNode<T>*;
    using AlNode = typename Base::AlNode;
    using AlNodeTraits = allocator_traits<allocator_type>;

public:
    // (1)
    forward_list() : forward_list(Alloc()) {
    }
    explicit forward_list(const Alloc& alloc) : Base(alloc) {
    }

    // (2)
    forward_list(size_type count, const T& val, const Alloc& alloc = Alloc())
        : Base(alloc) {
        insert_after(before_begin(), count, val);
    }

    // (3)
    explicit forward_list(size_type count, const Alloc& alloc = Alloc())
        : Base(alloc) {
        T val = T{};
        insert_after(before_begin(), count, val);
    }

    // (4)
    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    forward_list(InIter first, InIter last, const Alloc& alloc = Alloc())
        : Base(alloc) {
        insert_after(before_begin(), first, last);
    }

    // (5)
    forward_list(const forward_list& rhs)
        : Base(AlNodeTraits::select_on_container_copy_construction(
              rhs.get_allocator())) {
        insert_after(before_begin(), rhs.begin(), rhs.end());
    }

    // (5)
    forward_list(const forward_list& rhs, const Alloc& alloc) : Base(alloc) {
        insert_after(before_begin(), rhs.begin(), rhs.end());
    }

private:
    void constructMove(forward_list&& rhs, true_type) {
        swapADL(this->getHead(), rhs.getHead());
    }

    void constructMove(forward_list&& rhs, false_type) {
        if (this->getAlloc() == rhs.getAlloc())
            constructMove(tiny_stl::move(rhs), true_type{});
        else
            insert_after(before_begin(),
                         tiny_stl::make_move_iterator(rhs.begin()),
                         tiny_stl::make_move_iterator(rhs.end()));
    }

public:
    // (6)
    forward_list(forward_list&& rhs) noexcept
        : Base(tiny_stl::move(rhs.getAlloc())) {
        constructMove(tiny_stl::move(rhs), true_type{});
    }

    // (7)
    forward_list(forward_list&& rhs, const Alloc& alloc) : Base(alloc) {
        constructMove(tiny_stl::move(rhs), false_type{});
    }

    // (8)
    forward_list(std::initializer_list<T> ilist, const Alloc& alloc = Alloc())
        : Base(alloc) {
        insert_after(before_begin(), ilist.begin(), ilist.end());
    }

    forward_list& operator=(const forward_list& rhs) {
        assert(this != tiny_stl::addressof(rhs));

        if (AlNodeTraits::propagate_on_container_copy_assignment::value)
            this->getAlloc() = rhs.getAlloc();

        assign(rhs.begin(), rhs.end());
        return *this;
    }

    forward_list& operator=(forward_list&& rhs) noexcept(
        AlNodeTraits::is_always_equal::value) {
        assert(this != tiny_stl::addressof(rhs));

        clear();

        if (AlNodeTraits::propagate_on_container_move_assignment::value)
            this->getAlloc() = rhs.getAlloc();

        constructMove(
            tiny_stl::move(rhs),
            typename AlNodeTraits::propagate_on_container_move_assignment{});

        return *this;
    }

    forward_list& operator=(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void assign(size_type count, const T& val) {
        clear();
        insert_after(before_begin(), count, val);
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    void assign(InIter first, InIter last) {
        clear();
        insert_after(before_begin(), first, last);
    }

    void assign(std::initializer_list<T> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    allocator_type get_allocator() const {
        return this->getAlloc();
    }

    void clear() noexcept {
        NodePtr pNext;
        NodePtr p = this->getHead()->next;
        this->getHead()->next = nullptr;

        for (; p != nullptr; p = pNext) {
            pNext = p->next;
            this->freeNode(p);
        }
    }

    reference front() {
        return *begin();
    }

    const_reference front() const {
        return *begin();
    }

    iterator before_begin() noexcept {
        return iterator(this->getHead());
    }

    const_iterator before_begin() const noexcept {
        return const_iterator(this->getHead());
    }

    const_iterator cbefore_begin() const noexcept {
        return before_begin();
    }

    iterator begin() noexcept {
        return iterator(this->getHead()->next);
    }

    const_iterator begin() const noexcept {
        return const_iterator(this->getHead()->next);
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return iterator();
    }

    const_iterator end() const noexcept {
        return const_iterator();
    }

    const_iterator cend() const noexcept {
        return end();
    }

    bool empty() const noexcept {
        return begin() == end();
    }

    size_type max_size() const noexcept {
        return allocator_traits<AlNode>::max_size(this->getAlloc());
    }

    ~forward_list() noexcept {
        clear();
    }

private:
    size_type sizeAux() const {
        size_type n = 0;
        for (const_iterator p = begin(); p != end(); ++p, ++n) {
        }
        return n;
    }

    const_iterator beforeEndAux() const {
        const_iterator p = before_begin();
        for (const_iterator nextNode = p; ++nextNode != end(); p = nextNode) {
        }
        return p;
    }

public:
    iterator makeIter(const_iterator iter) const {
        return iterator(iter.ptr);
    }

    template <typename... Args>
    iterator emplace_after(const_iterator pos, Args&&... args) {
        NodePtr p = pos.ptr;
        NodePtr newNode =
            this->createNode(pos.ptr->next, tiny_stl::forward<Args>(args)...);
        p->next = newNode;

        return makeIter(++pos);
    }

    iterator insert_after(const_iterator pos, const T& val) {
        return emplace_after(pos, val);
    }

    iterator insert_after(const_iterator pos, T&& val) {
        return emplace_after(pos, tiny_stl::move(val));
    }

    iterator insert_after(const_iterator pos, size_type count, const T& val) {
        while (count--)
            pos = emplace_after(pos, val);
        return makeIter(pos);
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    iterator insert_after(const_iterator pos, InIter first, InIter last) {
        for (; first != last; ++first, ++pos)
            emplace_after(pos, *first);
        return makeIter(pos);
    }

    iterator insert_after(const_iterator pos, std::initializer_list<T> ilist) {
        return insert_after(pos, ilist.begin(), ilist.end());
    }

    void push_front(const T& val) {
        emplace_after(before_begin(), val);
    }

    void push_front(T&& val) {
        emplace_after(before_begin(), tiny_stl::move(val));
    }

    iterator erase_after(const_iterator pos) {
        NodePtr cur = pos.ptr;

        assert(pos.ptr->next != nullptr);

        ++pos;
        NodePtr pFree = pos.ptr;
        cur->next = pFree->next;
        ++pos; // return value
        this->freeNode(pFree);

        return makeIter(pos);
    }

    // erase the elements in the range (first, last)
    iterator erase_after(const_iterator first, const_iterator last) {
        if (first == before_begin() && last == end()) {
            clear();
            return end();
        } else {
            assert(first != end());
            if (first != last) {
                const_iterator nextNode = first;
                for (++nextNode; nextNode != last;)
                    nextNode = erase_after(first);
            }
            return makeIter(last);
        }
    }

    void pop_front() {
        erase_after(before_begin());
    }

    void resize(size_type count, const T& val) {
        size_type curSize = this->sizeAux();
        if (curSize < count)
            insert_after(this->beforeEndAux(), count - curSize, val);
        else if (curSize > count) {
            const_iterator tail = before_begin();
            while (count--)
                ++tail;
            erase_after(tail, end());
        }
    }

    void resize(size_type count) {
        T val = T{};
        resize(count, val);
    }

    void swap(forward_list& rhs) noexcept(
        allocator_traits<Alloc>::is_always_equal::value) {
        if (this != tiny_stl::addressof(rhs)) {
            swapAlloc(this->getAlloc(), rhs.getAlloc());
            swapADL(this->getHead(), rhs.getHead());
        }
    }

private:
    void spliceAfter(const_iterator pos, forward_list& rhs,
                     const_iterator first, const_iterator last) {
        assert(this->get_allocator() == rhs.get_allocator());
        const_iterator rhsBeforeEnd = first;
        const_iterator rhsEnd = rhsBeforeEnd;

        // Find the element before last iterator
        for (++rhsEnd; rhsEnd != last; ++rhsBeforeEnd, ++rhsEnd) {
            if (rhsEnd == rhs.end()) {
                assert(true);
                return;
            }
        }

        rhsBeforeEnd.ptr->next = pos.ptr->next;
        pos.ptr->next = first.ptr->next;
        first.ptr->next = last.ptr;
    }

    template <typename Cmp>
    void mergeAux(forward_list& rhs, const Cmp& cmp) {
        if (this != tiny_stl::addressof(rhs)) {
            // no prev pointer
            const_iterator first1 = this->before_begin(), next1 = this->begin(),
                           last1 = this->end();
            const_iterator first2 = rhs.before_begin(), next2 = rhs.begin(),
                           last2 = rhs.end();

            assert(tiny_stl::is_sorted(next1, last1));
            assert(tiny_stl::is_sorted(next2, last2));

            for (; next1 != last1 && next2 != last2; ++first1) {
                if (cmp(*next2, *next1)) // move *first2 to the front of *first
                    spliceAfter(first1, rhs, first2, ++next2);
                else
                    ++next1;
            }

            if (next2 != last2)
                spliceAfter(first1, rhs, first2, last2);
        }
    }

    // merge sort
    template <typename BinPred>
    void sortAux(iterator before_first, iterator last, BinPred pred,
                 size_type size) {
        if (size < 2)
            return;

        size_type mid_size = size >> 1;

        iterator mid = tiny_stl::next(before_first, 1 + mid_size); // init mid
        sortAux(before_first, mid, pred, mid_size); // sort first half

        // init before_mid
        iterator before_mid = tiny_stl::next(before_first, mid_size);
        sortAux(before_mid, last, pred, size - mid_size); // sort second half

        // Because the iterator mid does not fail
        // the previous sort will change the mid
        // therefore, update the itertor mid
        mid = tiny_stl::next(before_mid);
        iterator first = tiny_stl::next(before_first);

        // sort [first, mid) and [mid, last)
        for (;;) {
            if (pred(*mid, *first)) { // *mid < *first
                splice_after(before_first, *this, before_mid);
                ++before_first;
                mid = tiny_stl::next(before_mid);

                if (mid == last)
                    return;
            } else { // *mid >= *first
                ++before_first;
                ++first;

                if (first == mid)
                    return;
            }
        }
    }

public:
    void merge(forward_list& rhs) {
        mergeAux(rhs, tiny_stl::less<>{});
    }

    void merge(forward_list&& rhs) {
        mergeAux(rhs, tiny_stl::less<>{});
    }

    template <typename Cmp>
    void merge(forward_list& rhs, Cmp cmp) {
        mergeAux(rhs, cmp);
    }

    template <typename Cmp>
    void merge(forward_list&& rhs, Cmp cmp) {
        mergeAux(rhs, cmp);
    }

    // move all elements from rhs into after the pos
    void splice_after(const_iterator pos, forward_list& rhs) {
        assert(this != tiny_stl::addressof(rhs));
        spliceAfter(pos, rhs, rhs.before_begin(), rhs.end());
    }

    void splice_after(const_iterator pos, forward_list&& rhs) {
        splice_after(pos, rhs);
    }

    // move (first, first + 2) into after the pos
    void splice_after(const_iterator pos, forward_list& rhs,
                      const_iterator first) {
        const_iterator nextNode = first;
        ++nextNode;
        if (pos == first || pos == nextNode)
            return;

        spliceAfter(pos, rhs, first, ++nextNode);
    }

    void splice_after(const_iterator pos, forward_list&& rhs,
                      const_iterator first) {
        splice_after(pos, rhs, first);
    }

    // move (first, last) into after the pos
    void splice_after(const_iterator pos, forward_list& rhs,
                      const_iterator first, const_iterator last) {
        // pos can't be an element of (first, last)
#ifndef NDEBUG // DEBUG
        const_iterator nextNode = first;
        for (++nextNode; nextNode != last; ++nextNode)
            assert(pos != nextNode);
#endif // !NDEBUG

        spliceAfter(pos, rhs, first, last);
    }

    void splice_after(const_iterator pos, forward_list&& rhs,
                      const_iterator first, const_iterator last) {
        splice_after(pos, rhs, first, last);
    }

    template <typename UnaryPred>
    void remove_if(UnaryPred pred) {
        iterator before = before_begin();

        for (iterator first = begin(); first != end();) {
            if (pred(*first))
                first = erase_after(before);
            else {
                ++before;
                ++first;
            }
        }
    }

    void remove(const T& val) {
        remove_if([&val](const T& elem) { return val == elem; });
    }

    void reverse() noexcept {
        if (!empty()) {
            const_iterator beforeEnd = beforeEndAux();
            const_iterator first = begin();
            const_iterator nextNode = first;
            for (; first != beforeEnd; first = nextNode) {
                ++nextNode;
                this->getHead()->next = nextNode.ptr;
                first.ptr->next = beforeEnd.ptr->next;
                beforeEnd.ptr->next = first.ptr;
            }
        }
    }

    template <typename BinPred>
    void unique(BinPred pred) {
        if (empty())
            return;
        const_iterator first = begin();
        const_iterator nextNode = first;

        for (++nextNode; nextNode != end();) {
            if (pred(*first, *nextNode))
                nextNode = erase_after(first);
            else
                first = nextNode++;
        }
    }

    void unique() {
        unique(tiny_stl::equal_to<>{});
    }

    template <typename Cmp>
    void sort(Cmp cmp) {
        sortAux(before_begin(), end(), cmp, this->sizeAux());
    }

    void sort() {
        sort(tiny_stl::less<>{});
    }
}; // class forward_list<T, Alloc>

template <typename T, typename Alloc>
inline bool operator==(const forward_list<T, Alloc>& lhs,
                       const forward_list<T, Alloc>& rhs) {
    return tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, typename Alloc>
inline bool operator!=(const forward_list<T, Alloc>& lhs,
                       const forward_list<T, Alloc>& rhs) {
    return (!(lhs == rhs));
}

template <typename T, typename Alloc>
inline bool operator<(const forward_list<T, Alloc>& lhs,
                      const forward_list<T, Alloc>& rhs) {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename T, typename Alloc>
inline bool operator<=(const forward_list<T, Alloc>& lhs,
                       const forward_list<T, Alloc>& rhs) {
    return (!(rhs < lhs));
}

template <typename T, typename Alloc>
inline bool operator>(const forward_list<T, Alloc>& lhs,
                      const forward_list<T, Alloc>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Alloc>
inline bool operator>=(const forward_list<T, Alloc>& lhs,
                       const forward_list<T, Alloc>& rhs) {
    return (!(lhs < rhs));
}

template <typename T, typename Alloc>
void swap(forward_list<T, Alloc>& lhs,
          forward_list<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace tiny_stl
