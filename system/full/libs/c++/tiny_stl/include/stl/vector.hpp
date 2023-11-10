// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "memory.hpp"
#include <initializer_list>

namespace tiny_stl {

template <typename T>
struct VectorConstIterator {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;
    using Self = VectorConstIterator<T>;

    T* ptr;

    VectorConstIterator() : ptr() {
    }

    VectorConstIterator(T* p) : ptr(p) {
    }

    reference operator*() const {
        return *ptr;
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    Self& operator++() {
        ++ptr;
        return *this;
    }

    Self operator++(int) const {
        Self tmp = *this;
        ++*this;
        return tmp;
    }

    Self& operator--() {
        --ptr;
        return *this;
    }

    Self operator--(int) const {
        Self tmp = *this;
        --*this;
        return tmp;
    }

    Self& operator+=(difference_type offset) {
        ptr += offset;
        return *this;
    }

    Self operator+(difference_type offset) const {
        Self tmp = *this;
        return tmp += offset;
    }

    Self& operator-=(difference_type offset) {
        ptr -= offset;
        return *this;
    }

    Self operator-(difference_type offset) const {
        Self tmp = *this;
        return tmp -= offset;
    }

    difference_type operator-(const Self& rhs) const {
        return ptr - rhs.ptr;
    }

    reference operator[](difference_type offset) const {
        return *(ptr + offset);
    }

    bool operator==(const Self& rhs) const {
        return this->ptr == rhs.ptr;
    }

    bool operator!=(const Self& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(const Self& rhs) const {
        return this->ptr < rhs.ptr;
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
}; // class VectorConstIterator<T>

template <typename T>
inline VectorConstIterator<T>
operator+(typename VectorConstIterator<T>::difference_type offset,
          VectorConstIterator<T> iter) {
    return iter += offset;
}

template <typename T>
struct VectorIterator : VectorConstIterator<T> {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    using Base = VectorConstIterator<T>;
    using Self = VectorIterator<T>;

    VectorIterator() : Base() {
    }

    VectorIterator(T* p) : Base(p) {
    }

    reference operator*() const {
        return const_cast<reference>(Base::operator*());
    }

    pointer operator->() const {
        return const_cast<pointer>(Base::operator->());
    }

    Self& operator++() {
        ++*static_cast<Base*>(this);
        return *this;
    }

    Self operator++(int) {
        Self tmp = *this;
        ++*this;
        return tmp;
    }

    Self& operator--() {
        --*static_cast<Base*>(this);
        return *this;
    }

    Self operator--(int) {
        Self tmp = *this;
        --*this;
        return tmp;
    }

    Self& operator+=(difference_type offset) {
        *static_cast<Base*>(this) += offset;
        return *this;
    }

    Self operator+(difference_type offset) const {
        Self tmp = *this;
        return tmp += offset;
    }

    Self& operator-=(difference_type offset) {
        return (*this += (-offset));
    }

    Self operator-(difference_type offset) const {
        Self tmp = *this;
        return tmp -= offset;
    }

    difference_type operator-(const Self& rhs) const {
        return this->ptr - rhs.ptr;
    }

    reference operator[](std::ptrdiff_t offset) const {
        return *(*this + offset);
    }
}; // class VectorIterator<T>

template <typename T>
inline VectorIterator<T>
operator+(typename VectorIterator<T>::difference_type offset,
          VectorIterator<T> iter) {
    return iter += offset;
}

template <typename T, typename Alloc>
class VectorBase {
public:
    using value_type = T;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = VectorIterator<T>;
    using const_iterator = VectorConstIterator<T>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;
    using allocator_type = Alloc;

protected:
    allocator_type alloc;

    T* first;
    T* last;
    T* end_of_storage;

public:
    VectorBase(const Alloc& a) : alloc(a), first(), last(), end_of_storage() {
    }

    T* allocateAux(std::size_t n) {
        return alloc.allocate(n);
    }

    void deallocateAux(T* p, std::size_t n) {
        if (p != pointer())
            alloc.deallocate(p, n);
    }

    allocator_type& getAllocator() noexcept {
        return alloc;
    }

    const allocator_type& getAllocator() const noexcept {
        return alloc;
    }

    ~VectorBase() {
        deallocateAux(first, end_of_storage - first);
    }
}; // class VectorBase<T, Alloc>

template <typename T, typename Alloc = allocator<T>>
class vector : public VectorBase<T, Alloc> {
public:
    static_assert(tiny_stl::is_same_v<T, typename Alloc::value_type>,
                  "Alloc::value_type is not the same as T");

private:
    using Base = VectorBase<T, Alloc>;

public:
    using allocator_type = typename Base::allocator_type;
    using size_type = typename Base::size_type;
    using difference_type = typename Base::difference_type;
    using pointer = typename Base::pointer;
    using const_pointer = typename Base::const_pointer;
    using reference = typename Base::reference;
    using const_reference = typename Base::const_reference;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

private:
    using Base::alloc;
    using Base::end_of_storage;
    using Base::first;
    using Base::last;

private:
    // only allocate
    bool allocAux(size_type newCapacity) {
        this->first = pointer();
        this->last = pointer();
        this->end_of_storage = pointer();

        if (newCapacity == 0)
            return false;

        if (newCapacity >= max_size())
            xLength();

        this->first = this->allocateAux(newCapacity);
        this->last = this->first;
        this->end_of_storage = this->first + newCapacity;
        return true;
    }

    // construct
    pointer fillHelper(pointer dest, size_type count, const_reference val) {
        return uninitializedAllocFillN(dest, count, val, this->alloc);
    }

    // default construct
    pointer defaultConstruct(pointer dest, size_type count) {
        return uninitializedAllocDefaultN(dest, count, this->alloc);
    }

    // move construct
    void moveOrCopyAux(pointer xfirst, pointer xlast, pointer newFirst,
                       true_type) {
        uninitializedAllocMove(xfirst, xlast, newFirst, this->alloc);
    }

    // copy construct
    void moveOrCopyAux(pointer xfirst, pointer xlast, pointer newFirst,
                       false_type) {
        uninitializedAllocCopy(xfirst, xlast, newFirst, this->alloc);
    }

    void moveOrCopy(pointer xfirst, pointer xlast, pointer newFirst) {
        moveOrCopyAux(xfirst, xlast, newFirst,
                      typename tiny_stl::disjunction<
                          is_nothrow_move_constructible<T>,
                          negation<is_copy_constructible<T>>>::type());
    }

    pointer moveAux(pointer xfirst, pointer xlast, pointer newFirst) {
        return uninitializedAllocMove(xfirst, xlast, newFirst, this->alloc);
    }

    template <typename Iter> // ? Iter
    pointer copyAux(Iter xfirst, Iter xlast, pointer newFirst) {
        return uninitializedAllocCopy(xfirst, xlast, newFirst, this->alloc);
    }

    // destory and deallocate old array
    void tidy() {
        if (this->first != pointer()) {
            destroyAllocRange(this->first, this->last, this->alloc);
            this->alloc.deallocate(this->first, capacity());

            this->first = pointer();
            this->last = pointer();
            this->end_of_storage = pointer();
        }
    }

public:
    // (1)
    vector() noexcept(noexcept(Alloc())) : vector(Alloc()) {
    }
    explicit vector(const Alloc& alloc) noexcept : Base(alloc) {
    }

    // (2)
    vector(size_type count, const T& val, const Alloc& alloc = Alloc())
        : Base(alloc) {
        try {
            if (allocAux(count))
                this->last = fillHelper(this->first, count, val);
        } catch (...) {
            tidy();
            throw;
        }
    }

    // (3)
    explicit vector(size_type count, const Alloc& alloc = Alloc())
        : Base(alloc) {
        try {
            if (allocAux(count))
                this->last = defaultConstruct(this->first, count);
        } catch (...) {
            tidy();
            throw;
        }
    }

private:
    template <typename InIter>
    void rangeConstruct(InIter xfirst, InIter xlast, input_iterator_tag) {
        // input_iterator has no distance function
        try {
            for (; xfirst != xlast; ++xfirst)
                emplace_back(*xfirst); // copy
        } catch (...) {
            tidy();
            throw;
        }
    }

    template <typename FwdIter>
    void rangeConstruct(FwdIter xfirst, FwdIter xlast, forward_iterator_tag) {
        size_type size =
            static_cast<size_type>(tiny_stl::distance(xfirst, xlast));
        try {
            if (allocAux(size))
                this->last = copyAux(xfirst, xlast, this->first);
        } catch (...) {
            tidy();
            throw;
        }
    }

public:
    // (4)
    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    vector(InIter xfirst, InIter xlast, const Alloc& alloc = Alloc())
        : Base(alloc) {
        rangeConstruct(xfirst, xlast,
                       typename iterator_traits<InIter>::iterator_category{});
    }

    // (5)
    vector(const vector& rhs)
        : Base(allocator_traits<Alloc>::select_on_container_copy_construction(
              rhs.alloc)) {
        try {
            if (allocAux(rhs.size()))
                this->last = copyAux(rhs.first, rhs.last, this->first);
        } catch (...) {
            tidy();
            throw;
        }
    }

    // (5) alloc
    vector(const vector& rhs, const Alloc& alloc) : Base(alloc) {
        try {
            if (allocAux(rhs.size()))
                this->last = copyAux(rhs.first, rhs.last, this->first);
        } catch (...) {
            tidy();
            throw;
        }
    }

private:
    void constructMove(vector&& rhs, true_type) noexcept {
        this->first = rhs.first;
        this->last = rhs.last;
        this->end_of_storage = rhs.end_of_storage;

        rhs.first = pointer();
        rhs.last = pointer();
        rhs.end_of_storage = pointer();
    }

    void constructMove(vector&& rhs, false_type) {
        if (this->alloc == rhs.alloc)
            constructMove(tiny_stl::move(rhs), true_type{});
        else if (allocAux(rhs.size()))
            this->last = moveAux(rhs.first, rhs.last, this->first);
    }

public:
    // (6)
    vector(vector&& rhs) noexcept : Base(tiny_stl::move(rhs.alloc)) {
        constructMove(tiny_stl::move(rhs), true_type{});
    }

    // (7)
    vector(vector&& rhs, const Alloc& alloc) : Base(alloc) {
        // FIXME, no strong exception
        constructMove(rhs.first, rhs.last,
                      typename allocator_traits<Alloc>::is_always_equal{});
    }

    // (8)
    vector(std::initializer_list<T> ilist, const Alloc& alloc = Alloc())
        : Base(alloc) {
        rangeConstruct(ilist.begin(), ilist.end(),
                       random_access_iterator_tag{});
    }

    ~vector() {
        tidy();
    }

private:
    template <typename InIter>
    void assignCopyRange(InIter xfirst, InIter xlast, input_iterator_tag) {
        destroyRange(this->first, this->last);

        InIter newLast = this->first;
        for (; xfirst != xlast && newLast != this->last; ++xfirst, ++newLast)
            *newLast = *xfirst;

        this->last = newLast;
        for (; xfirst != xlast; ++xfirst)
            emplace_back(*xfirst);
    }

    template <typename FwdIter>
    void assignCopyRange(FwdIter xfirst, FwdIter xlast, forward_iterator_tag) {
        const size_type newSize =
            static_cast<size_type>(tiny_stl::distance(xfirst, xlast));
        const size_type oldCapacity = capacity();

        try {
            if (newSize > oldCapacity) { // reallocate
                if (newSize > max_size())
                    xLength();

                if (this->first != pointer())
                    tidy();

                allocAux(newSize);
                this->last = copyAux(xfirst, xlast, this->first);
            } else { // no reallocate
                destroyRange(this->first, this->last);
                this->last = this->copyAux(xfirst, xlast, this->first);
            }
        } catch (...) {
            tidy();
            throw;
        }
    }

    void assignMove(vector&& rhs, true_type) noexcept {
        constructMove(tiny_stl::move(rhs), true_type{});
    }

    void assignMove(vector&& rhs, false_type) {
        if (this->alloc == rhs.alloc)
            constructMove(tiny_stl::move(rhs), true_type{});

        // Move individually
        const size_type newSize = rhs.size();
        const size_type oldCapacity = this->capacity();

        try {
            if (newSize > oldCapacity) { // reallocate
                if (newSize > max_size())
                    xLength();

                if (this->first != pointer())
                    tidy();

                allocAux(newSize);
                this->last = moveAux(rhs.first, rhs.last, this->first);
            } else { // no reallocate
                destroyRange(this->first, this->last);
                this->last = moveAux(rhs.first, rhs.last, this->first);
            }
        } catch (...) {
            tidy();
            throw;
        }
    }

public:
    void assign(size_type n, const T& val) {
        const size_type oldCapacity = capacity();

        try {
            if (n > oldCapacity) { // reallocate
                if (n > max_size())
                    xLength();

                if (this->first != pointer())
                    tidy();

                allocAux(n);
                this->last = fillHelper(this->first, n, val);
            } else { // no reallocate
                destroyRange(this->first, this->last);
                this->last = fillHelper(this->first, n, val);
            }
        } catch (...) {
            tidy();
            throw;
        }
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    void assign(InIter xfirst, InIter xlast) {
        assignCopyRange(xfirst, xlast,
                        typename iterator_traits<InIter>::iterator_category{});
    }

    void assign(std::initializer_list<T> ilist) {
        assignCopyRange(ilist.begin(), ilist.end(),
                        random_access_iterator_tag{});
    }

    vector& operator=(const vector& rhs) {
        assert(this != tiny_stl::addressof(rhs));

        if (this->alloc != rhs.alloc)
            tidy(); // this->alloc deallocate elements

        if (allocator_traits<
                Alloc>::propagate_on_container_copy_assignment::value)
            this->alloc = rhs.alloc;

        assign(rhs.first, rhs.last);
        return *this;
    }

    vector& operator=(vector&& rhs) noexcept(
        allocator_traits<
            Alloc>::propagate_on_container_move_assignment::value ||
        allocator_traits<Alloc>::is_always_equal::value) {
        assert(this != tiny_stl::addressof(rhs));

        if (allocator_traits<
                Alloc>::propagate_on_container_move_assignment::value)
            this->alloc = rhs.alloc;

        assignMove(
            tiny_stl::move(rhs),
            disjunction<typename allocator_traits<
                            Alloc>::propagate_on_container_move_assignment,
                        typename allocator_traits<Alloc>::is_always_equal>{});

        return *this;
    }

    vector& operator=(std::initializer_list<T> ilist) {
        assignCopyRange(ilist.begin(), ilist.end(),
                        random_access_iterator_tag{});
        return *this;
    }

    allocator_type get_allocator() const {
        return static_cast<allocator_type>(this->alloc);
    }

    T& at(size_type pos) {
        if (pos >= size())
            xRange();

        return this->first[pos];
    }

    const T& at(size_type pos) const {
        if (pos >= size())
            xRange();

        return this->first[pos];
    }

    T& operator[](size_type pos) {
        assert(pos < size());
        return this->first[pos];
    }

    const T& operator[](size_type pos) const {
        assert(pos < size());
        return this->first[pos];
    }

    T& front() {
        assert(!empty());
        return *this->first;
    }

    const T& front() const {
        assert(!empty());
        return *this->first;
    }

    T& back() {
        assert(!this->empty());
        return this->last[-1];
    }

    const T& back() const {
        assert(!this->empty());
        return this->last[-1];
    }

    T* data() noexcept {
        return this->first;
    }

    const T* data() const noexcept {
        return this->first;
    }

    iterator begin() noexcept {
        return iterator(first);
    }

    const_iterator begin() const noexcept {
        return const_iterator(first);
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return iterator(last);
    }

    const_iterator end() const noexcept {
        return iterator(last);
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
        return rend();
    }

private:
    void updatePointer(const pointer newFirst, size_type newSize,
                       size_type newCapacity) {
        if (this->first != pointer()) {
            destroyAllocRange(this->first, this->last, this->alloc);
            this->alloc.deallocate(this->first, capacity());
        }

        this->first = newFirst;
        this->last = newFirst + newSize;
        this->end_of_storage = newFirst + newCapacity;
    }

    void reallocAndInit(size_type newCapacity) {
        const size_type newSize = size();
        const pointer newFirst = this->alloc.allocate(newCapacity);

        try {
            moveOrCopy(this->first, this->last, newFirst);
        } catch (...) {
            tidy();
            throw;
        }

        // destroy/deallocate old elements, update new pointer
        updatePointer(newFirst, newSize, newCapacity);
    }

    size_type capacityGrowth(size_type newSize) const {
        const size_type oldCapacity = capacity();

        if ((oldCapacity << 1) > max_size())
            return newSize;

        const size_type newCapacity = (oldCapacity << 1);

        return newCapacity < newSize ? newSize : newCapacity;
    }

public:
    bool empty() const noexcept {
        return begin() == end();
    }

    size_type size() const noexcept {
        return static_cast<size_type>(tiny_stl::distance(begin(), end()));
    }

    size_type max_size() const noexcept {
        return sizeof(T) == 1 ? static_cast<size_type>(-1) >> 1
                              : static_cast<size_type>(-1) / sizeof(T);
    }

    void reserve(size_type newcapacity) {
        if (newcapacity > capacity()) {
            if (newcapacity > max_size())
                xLength();

            reallocAndInit(newcapacity);
        }
        // else do nothing
    }

    size_type capacity() const noexcept {
        return this->end_of_storage - this->first;
    }

    void shrink_to_fit() {
        if (size() < capacity()) {
            if (empty())
                tidy(); // no destroy, only deallocate
            else
                reallocAndInit(size());
        }
    }

public:
    void clear() noexcept {
        destroyAllocRange(this->first, this->last, this->alloc);
        this->last = this->first;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (this->last != this->end_of_storage) { // has unused capacity
            allocator_traits<Alloc>::construct(
                this->alloc, this->last, tiny_stl::forward<Args>(args)...);
            ++this->last;
        } else { // reallocate
            const size_type oldSize = size();
            if (oldSize == max_size())
                xLength();

            const size_type newSize = oldSize + 1;

            // normal: capacity <<= 1
            const size_type newCapacity = capacityGrowth(newSize);

            try {
                const pointer newFirst = this->alloc.allocate(newCapacity);

                allocator_traits<Alloc>::construct(
                    this->alloc, tiny_stl::addressof(*(newFirst + oldSize)),
                    tiny_stl::forward<Args>(args)...);

                moveOrCopy(this->first, this->last, newFirst);
                updatePointer(newFirst, newSize, newCapacity);
            } catch (...) {
                tidy();
                throw;
            }
        }
    }

    void push_back(const_reference val) {
        emplace_back(val);
    }

    void push_back(T&& val) {
        emplace_back(tiny_stl::move(val));
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        assert(pos.ptr >= this->first && pos.ptr <= this->last);
        const bool is_back = pos.ptr == this->last;
        const size_type offset = pos.ptr - this->first;

        if (this->last == this->end_of_storage) { // reallocate
            const size_type oldSize = size();
            if (oldSize == max_size())
                xLength();

            // reallocate
            const size_type newSize = oldSize + 1;
            const size_type newCapacity = capacityGrowth(newSize);

            try {
                const pointer newFirst = this->alloc.allocate(newCapacity);

                // construct
                allocator_traits<Alloc>::construct(
                    this->alloc, tiny_stl::addressof(*(newFirst + offset)),
                    tiny_stl::forward<Args>(args)...);

                if (is_back) { // Strong exception guarantee
                    moveOrCopy(this->first, this->last, newFirst);
                } else {
                    moveAux(this->first, pos.ptr, newFirst);
                    moveAux(pos.ptr, this->last, newFirst + offset + 1);
                }

                updatePointer(newFirst, newSize, newCapacity);
            } catch (...) {
                tidy();
                throw;
            }
        } else if (is_back) { // no reallocate, and emplace at back
            allocator_traits<Alloc>::construct(
                this->alloc, tiny_stl::addressof(*this->last),
                tiny_stl::forward<Args>(args)...);
            ++this->last;
        } else { // no reallocate, move old elements
            pointer oldLast = this->last;
            for (; pos.ptr != oldLast; --oldLast)
                *oldLast = tiny_stl::move(oldLast[-1]);

            T obj(tiny_stl::forward<Args>(args)...);
            ++this->last;
            *oldLast = tiny_stl::move(obj);
        }
        return begin() + offset;
    }

    iterator insert(const_iterator pos, const T& val) {
        return emplace(pos, val);
    }

    iterator insert(const_iterator pos, T&& val) {
        return emplace(pos, tiny_stl::move(val));
    }

    iterator insert(const_iterator pos, size_type n, const T& val) {
        assert(pos.ptr >= this->first && pos.ptr <= this->last);

        if (n == 1 && pos.ptr == this->last)
            return emplace(pos, val); // Strong exception guarantee

        const size_type offset = pos.ptr - this->first;
        if (n == 0) {
            // do nothing
        } else if (n >
                   static_cast<size_type>(this->end_of_storage - this->last)) {
            // reallocate
            const size_type oldSize = size();
            if (n + oldSize > max_size())
                xLength();

            const size_type newSize = oldSize + n;
            const size_type newCapacity = capacityGrowth(newSize);

            try {
                const pointer newFirst = this->alloc.allocate(newCapacity);
                fillHelper(newFirst + offset, n, val);
                moveAux(this->first, pos.ptr, newFirst);
                moveAux(pos.ptr, this->last, newFirst + offset + n);
                updatePointer(newFirst, newSize, newCapacity);
            } catch (...) {
                tidy();
                throw;
            }
        } else { // no reallocate
            const pointer oldLast = this->last;
            const size_type number_move = oldLast - pos.ptr;

            try {
                if (n >= number_move) { // no move backward
                    this->last = fillHelper(oldLast, n - number_move, val);
                    this->last =
                        moveAux(pos.ptr, pos.ptr + number_move, this->last);
                    fill(pos.ptr, oldLast, val);
                } else { // move backward
                    this->last = oldLast + n;
                    pointer p = const_cast<pointer>(oldLast + (n - 1));
                    for (size_type i = 0; i < number_move; ++i, --p)
                        *p =
                            tiny_stl::move(p[-static_cast<difference_type>(n)]);

                    fill(pos.ptr, p, val);
                }
            } catch (...) {
                tidy();
                throw;
            }
        }

        return begin() + offset;
    }

private:
    template <typename InIter>
    void insertRangeAux(const_iterator pos, InIter xfirst, InIter xlast,
                        input_iterator_tag) {
        if (xfirst == xlast) // empty range
            return;

        // 1 2 3 6 insert 4 5 => 1 2 3 4 5 6
        for (; xfirst != xlast; ++xfirst)
            emplace_back(*xfirst); // 1 2 3 6 4 5

        const size_type offset = pos.ptr - this->first;
        const size_type oldSize = size();
        // 1 2 3 6 4 5
        //       f m   l
        // rotate =>  1 2 3 4 5 6
        rotate(this->first + offset, this->first + oldSize, this->last);
    }

    template <typename FwdIter>
    void insertRangeAux(const_iterator pos, FwdIter xfirst, FwdIter xlast,
                        forward_iterator_tag) {
        const size_type n =
            static_cast<size_type>(tiny_stl::distance(xfirst, xlast));
        const size_type offset = pos.ptr - this->first;

        if (n == 1 && pos.ptr == this->last)
            emplace(pos.ptr, *xfirst);

        if (n == 0) {
            // do nothing
        } else if (n >
                   static_cast<size_type>(this->end_of_storage - this->last)) {
            // reallocate
            const size_type oldSize = size();
            if (n + oldSize > max_size())
                xLength();

            const size_type newSize = oldSize + n;
            const size_type newCapacity = capacityGrowth(newSize);

            try {
                const pointer newFirst = this->alloc.allocate(newCapacity);

                copyAux(xfirst, xlast, newFirst + offset);
                moveAux(this->first, pos.ptr, newFirst);
                moveAux(pos.ptr, this->last, newFirst + offset + n);

                updatePointer(newFirst, newSize, newCapacity);
            } catch (...) {
                tidy();
                throw;
            }
        } else { // no reallocate
            const pointer oldLast = this->last;
            const size_type number_move = oldLast - pos.ptr;
            if (n >= number_move) { // no move backward
                const pointer newPos = this->first + (offset + n);
                this->last = moveAux(pos.ptr, oldLast, newPos);

                try {
                    copyAux(xfirst, xlast, pos.ptr);
                } catch (...) {
                    // FIXME, move rollback
                    tidy();
                    throw;
                }
            } else { // move backward
                this->last = oldLast + n;
                pointer p = const_cast<pointer>(oldLast + (n - 1));
                for (size_type i = 0; i < number_move; ++i, --p)
                    *p = tiny_stl::move(p[-static_cast<difference_type>(n)]);

                try {
                    copyAux(xfirst, xlast, pos.ptr);
                } catch (...) {
                    // FIXME, move rollback
                    tidy();
                    throw;
                }
            }
        }
    }

public:
    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    iterator insert(const_iterator pos, InIter xfirst, InIter xlast) {
        assert(pos.ptr >= this->first && pos.ptr <= this->last);
        const size_type offset = pos.ptr - this->first;
        insertRangeAux(pos.ptr, xfirst, xlast,
                       typename iterator_traits<InIter>::iterator_category{});
        return this->first + offset;
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    iterator erase(const_iterator pos) {
        assert(pos.ptr >= this->first && pos.ptr < this->last);

        const size_type offset = pos.ptr - this->first;
        moveAux(pos.ptr + 1, this->last, pos.ptr);
        this->alloc.destroy(tiny_stl::addressof(*(this->last - 1)));
        --this->last;
        return this->first + offset;
    }

    iterator erase(const_iterator xfirst, const_iterator xlast) {
        assert(xfirst.ptr == xlast.ptr ||
               (xfirst.ptr >= this->first && xfirst.ptr < xlast.ptr &&
                xlast.ptr <= this->last));
        const size_type offset = xfirst.ptr - this->first;
        if (xfirst != xlast) {
            const pointer newLast = moveAux(xlast.ptr, this->last, xfirst.ptr);
            destroyRange(newLast, this->last);
            this->last = newLast;
        }
        return this->first + offset;
    }

    void pop_back() {
        assert(!empty());
        allocator_traits<Alloc>::destroy(this->alloc,
                                         tiny_stl::addressof(*(--this->last)));
    }

private:
    template <typename Lambda>
    void resizeHelper(size_type newSize, Lambda default_or_fill) {
        const size_type oldSize = size();
        const size_type oldCapacity = capacity();

        if (newSize > oldCapacity) { // reallocate
            if (newSize > max_size())
                xLength();

            size_type newCapacity = (newSize >> 1) + newSize;

            const pointer newFirst = nullptr;
            try {
                newFirst = this->alloc.allocate(newCapacity);
                moveOrCopy(this->first, this->last, newFirst);
            } catch (...) {
                tidy();
                throw;
            }

            // destroy/deallocate old elements, update new pointer
            updatePointer(newFirst, newSize, newCapacity);
        } else if (newSize < oldSize) { // update pointer, size = newSize
            const pointer newLast = this->first + newSize;
            destroyRange(newLast, this->last);
            this->last = newLast;
        } else if (newSize > oldSize) { // use lambda to append elements
            const pointer oldLast = this->last;
            this->last = default_or_fill(oldLast, newSize - oldSize);
        }
    }

public:
    void resize(size_type newSize) {
        auto lambda_default = [this](pointer oldLast, size_type n) {
            return defaultConstruct(oldLast, n);
        };
        resizeHelper(newSize, lambda_default);
    }

    void resize(size_type newSize, const T& val) {
        auto lambda_fill = [this, &val](pointer oldLast, size_type n) {
            return fillHelper(oldLast, n, val);
        };
        resizeHelper(newSize, lambda_fill);
    }

    void swap(vector& rhs) noexcept(
        noexcept(allocator_traits<Alloc>::propagate_on_container_swap::value ||
                 allocator_traits<Alloc>::is_always_equal::value)) {
        swapAlloc(this->alloc, rhs.alloc);
        swapADL(this->first, rhs.first);
        swapADL(this->last, rhs.last);
        swapADL(this->end_of_storage, rhs.end_of_storage);
    }

private:
    [[noreturn]] static void xLength() {
        throw "vector<T> too long";
    }

    [[noreturn]] static void xRange() {
        throw "invalid vector<T> subscript";
    }
}; // class vector<T>

template <typename T, typename Alloc>
inline bool operator==(const vector<T, Alloc>& lhs,
                       const vector<T, Alloc>& rhs) {
    return lhs.size() == rhs.size() &&
           tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Alloc>
inline bool operator!=(const vector<T, Alloc>& lhs,
                       const vector<T, Alloc>& rhs) {
    return (!(lhs == rhs));
}

template <typename T, typename Alloc>
inline bool operator<(const vector<T, Alloc>& lhs,
                      const vector<T, Alloc>& rhs) {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename T, typename Alloc>
inline bool operator<=(const vector<T, Alloc>& lhs,
                       const vector<T, Alloc>& rhs) {
    return (!(rhs < lhs));
}

template <typename T, typename Alloc>
inline bool operator>(const vector<T, Alloc>& lhs,
                      const vector<T, Alloc>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Alloc>
inline bool operator>=(const vector<T, Alloc>& lhs,
                       const vector<T, Alloc>& rhs) {
    return (!(lhs < rhs));
}

template <typename T, typename Alloc>
inline void swap(vector<T, Alloc>& lhs,
                 vector<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace tiny_stl
