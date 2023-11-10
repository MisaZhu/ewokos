// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "memory.hpp"
#include "string_view.hpp"
#include <initializer_list>

namespace tiny_stl {

template <typename T>
struct StringConstIterator {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;
    using difference_type = std::ptrdiff_t;
    using Self = StringConstIterator<T>;

    const T* ptr;

    StringConstIterator() = default;
    StringConstIterator(const T* p) : ptr(p) {
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

    Self operator++(int) {
        Self tmp = *this;
        ++*this;
        return tmp;
    }

    Self& operator--() {
        --ptr;
        return *this;
    }

    Self operator--(int) {
        Self tmp = *this;
        --*this;
        return tmp;
    }

    Self& operator+=(difference_type n) {
        ptr += n;
        return *this;
    }

    Self operator+(difference_type n) const {
        Self tmp = *this;
        return tmp += n;
    }

    Self& operator-=(difference_type n) {
        ptr -= n;
        return *this;
    }

    Self operator-(difference_type n) {
        Self tmp = *this;
        return tmp -= n;
    }

    difference_type operator-(const Self& rhs) const {
        return this->ptr - rhs.ptr;
    }

    reference operator[](difference_type n) const {
        return *(this->ptr + n);
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
}; // StringConstIterator<T>

template <typename T>
struct StringIterator : StringConstIterator<T> {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using difference_type = std::ptrdiff_t;
    using Base = StringConstIterator<T>;
    using Self = StringIterator<T>;

    StringIterator() = default;
    StringIterator(T* p) : Base(p) {
    }

    reference operator*() const {
        return const_cast<reference>(*this->ptr);
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
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

    Self& operator+=(difference_type n) {
        *static_cast<Base*>(this) += n;
        return *this;
    }

    Self& operator-=(difference_type n) {
        *static_cast<Base*>(this) -= n;
        return *this;
    }

    Self operator-(difference_type n) const {
        Self tmp = *this;
        return tmp -= n;
    }

    difference_type operator-(const Self& rhs) const {
        return this->ptr - rhs.ptr;
    }

    reference operator[](difference_type n) const {
        return *(this->ptr + n);
    }
}; // StringIterator<T>

template <typename CharT, typename Traits = std::char_traits<CharT>,
          typename Alloc = allocator<CharT>>
class basic_string {
public:
    static_assert(is_same<typename Traits::char_type, CharT>::value,
                  "char type error");

public:
    using traits_type = Traits;
    using value_type = CharT;
    using allocator_type = Alloc;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = StringIterator<value_type>;
    using const_iterator = StringConstIterator<value_type>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;
    using AllocTraits = allocator_traits<Alloc>;

    using Alty = RebindAllocType<Alloc, value_type>;

public:
    static const size_type npos = static_cast<size_type>(-1);

private:
    class StringValue {
    public:
        static_assert(sizeof(value_type) <= 16,
                      "size of value_type is too large");
        static constexpr const size_type kBufferSize = 16 / sizeof(value_type);
        static constexpr const size_type kBufferMask =
            sizeof(value_type) <= 1   ? 15
            : sizeof(value_type) <= 2 ? 7
            : sizeof(value_type) <= 4 ? 3
            : sizeof(value_type) <= 8 ? 1
                                      : 0;

    public:
        size_type size;
        size_type capacity;

        // short string optimization
        union Data {
            Data() : buf() {
            }
            ~Data() noexcept {
            }

            value_type buf[kBufferSize];
            pointer ptr;
            char placeholder[kBufferSize]; // unused
        } data;

        StringValue() : size(0), capacity(0), data() {
        }

        const value_type* getPtr() const {
            const value_type* ptr = data.ptr;
            if (isShortString()) {
                ptr = data.buf;
            }
            return ptr;
        }

        value_type* getPtr() {
            return const_cast<value_type*>(
                static_cast<const StringValue*>(this)->getPtr());
        }

        void checkIndex(const size_type idx) const {
            if (idx > size) {
                xRange();
            }
        }

        bool isShortString() const {
            return capacity < kBufferSize;
        }

        [[noreturn]] static void xRange() {
            throw "invalid tiny_stl::basic_string<CharT> index";
        }
    };

private:
    extra::compress_pair<Alloc, StringValue> allocVal;

private:
    struct EqualAllocator {};
    using PropagateAllocator = true_type;
    using NoPropagateAllocator = false_type;

    template <typename AllocX>
    using AllocMoveType = conditional_t<
        allocator_traits<AllocX>::is_always_equal::value, EqualAllocator,
        typename allocator_traits<
            AllocX>::propagate_on_container_move_assignment::type>;

public:
    explicit basic_string(const Alloc& a) : allocVal(a) {
        initEmpty();
    }

    basic_string() noexcept(noexcept(Alloc())) : basic_string(Alloc()) {
    }

    basic_string(size_type count, value_type ch, const Alloc& a = Alloc())
        : allocVal(a) {
        initEmpty();
        init(count, ch);
    }

    basic_string(const basic_string& rhs, size_type pos,
                 const Alloc& a = Alloc())
        : basic_string(rhs, pos, npos, a) {
    }

    basic_string(const basic_string& rhs, size_type pos, size_type count,
                 const Alloc& a = Alloc())
        : allocVal(a) {
        initEmpty(); // for setting capacity
        init(rhs, pos, count);
    }

    basic_string(const value_type* str, size_type count, const Alloc& a)
        : allocVal(a) {
        initEmpty(); // for setting capacity
        init(str, count);
    }

    basic_string(const value_type* str, const Alloc& a = Alloc())
        : allocVal(a) {
        initEmpty(); // for setting capacity
        init(str);
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    basic_string(InIter first, InIter last, const Alloc& a = Alloc())
        : allocVal(a) {
        initEmpty(); // for setting capacity
        // FIXME: if InIter is not [const] value_type*
        constructRange(first, last,
                       typename iterator_traits<InIter>::iterator_category{});
    }

    basic_string(const basic_string& rhs)
        : basic_string(rhs, AllocTraits::select_on_container_copy_construction(
                                rhs.getAlloc())) {
    }

    basic_string(const basic_string& rhs, const Alloc& a) : allocVal(a) {
        constructCopy(rhs);
    }

    basic_string(basic_string&& rhs) noexcept
        : allocVal(tiny_stl::move(rhs.getAlloc())) {
        constructMove(rhs);
    }

    basic_string(basic_string&& rhs,
                 const Alloc& a) noexcept(AllocTraits::is_always_equal::value)
        : allocVal(a) {
        if IFCONSTEXPR (AllocTraits::is_always_equal::value) {
            if (getAlloc() != rhs.getAlloc()) {
                constructCopy(rhs);
                return;
            }
        }
        constructMove(rhs);
    }

    basic_string(std::initializer_list<value_type> ilist,
                 const Alloc& a = Alloc())
        : allocVal(a) {
        initEmpty(); // for setting capacity
        checkLength(ilist.size());
        init(ilist.begin(), ilist.size());
    }

    ~basic_string() {
        tidy();
    }

public:
    basic_string& operator=(const basic_string& rhs) {
        if (this != tiny_stl::addressof(rhs)) {
            copyAlloc(getAlloc(), rhs.getAlloc());
            init(rhs.getVal().getPtr(), rhs.getVal().size);
        }

        return *this;
    }

    basic_string& operator=(basic_string&& rhs) noexcept(
        noexcept(assignMove(rhs, AllocMoveType<Alty>{}))) {
        if (this != tiny_stl::addressof(rhs)) {
            assignMove(rhs, AllocMoveType<Alty>{});
        }

        return *this;
    }

    basic_string& operator=(const value_type* str) {
        return init(str);
    }

    basic_string& operator=(value_type ch) {
        getVal().size = 1;
        pointer ptr = getVal().getPtr();
        Traits::assign(ptr[0], ch);
        Traits::assign(ptr[1], value_type{});
        return *this;
    }

    basic_string& operator=(std::initializer_list<CharT> ilist) {
        checkLength(ilist.size());
        return init(ilist.begin(), ilist.size());
    }

    basic_string& assign(size_type count, value_type ch) {
        return init(count, ch);
    }

    basic_string& assign(const basic_string& rhs) {
        *this = rhs;
        return *this;
    }

    basic_string& assign(const basic_string& rhs, size_type pos,
                         size_type count = npos) {
        return init(rhs, pos, count);
    }

    basic_string& assing(basic_string&& rhs) noexcept(
        noexcept(AllocTraits::propagate_on_container_move_assignment::value ||
                 AllocTraits::is_always_equal::value)) {
        *this = tiny_stl::move(rhs);
        return *this;
    }

    basic_string& assign(const value_type* str, size_type count) {
        return init(str, count);
    }

    basic_string& assign(const value_type* str) {
        return init(str);
    }

    template <typename InIter, typename = enable_if_t<is_iterator_v<InIter>>>
    basic_string& assign(InIter first, InIter last) {
        // FIXME: if InIter is not [const] value_type*
        constructRange(first, last,
                       typename iterator_traits<InIter>::iterator_category{});
        return *this;
    }

    basic_string& assign(std::initializer_list<value_type> ilist) {
        checkLength(ilist.size());
        return assign(ilist.begin(), ilist.size());
    }

private:
    template <typename AllocX>
    void copyAllocAux(AllocX& lhs, const AllocX& rhs, true_type) noexcept {
        lhs = rhs;
    }

    template <typename AllocX>
    void copyAllocAux(AllocX& lhs, const AllocX& rhs, false_type) noexcept {
    }

    template <typename AllocX>
    void copyAlloc(AllocX& lhs, const AllocX& rhs) noexcept {
        copyAllocAux(lhs, rhs,
                     typename allocator_traits<
                         AllocX>::propagate_on_container_copy_assignment{});
    }

    template <typename AllocX>
    void moveAllocAux(AllocX& lhs, AllocX& rhs, true_type) noexcept {
        lhs = tiny_stl::move(rhs);
    }

    template <typename AllocX>
    void moveAllocAux(AllocX& lhs, AllocX& rhs, false_type) noexcept {
    }

    template <typename AllocX>
    void moveAlloc(AllocX& lhs, AllocX& rhs) {
        typename allocator_traits<
            AllocX>::propagate_on_container_move_assignment tag{};
        moveAllocAux(lhs, rhs, tag);
    }

    void assignMove(basic_string& rhs, EqualAllocator) noexcept {
        tidy();
        moveAlloc(getAlloc(), rhs.getAlloc());
        constructMove(rhs);
    }

    void assignMove(basic_string& rhs, PropagateAllocator) noexcept {
        if (getAlloc() == rhs.getAlloc()) {
            assignMove(rhs, EqualAllocator{});
        } else {
            moveAlloc(getAlloc(), rhs.getAlloc());
            constructMove(rhs);
        }
    }

    void assignMove(basic_string& rhs, NoPropagateAllocator) {
        if (getAlloc() == rhs.getAlloc()) {
            assignMove(rhs, EqualAllocator{});
        } else {
            init(rhs.getVal().getPtr(), rhs.getVal().size);
        }
    }

private:
    void constructCopy(const basic_string& rhs) {
        auto& rhsValue = rhs.getVal();
        const size_type rhsSize = rhsValue.size;
        const value_type* rhsPtr = rhsValue.getPtr();
        auto& value = getVal();
        if (rhsSize < StringValue::kBufferSize) {
            Traits::move(value.data.buf, rhsPtr, StringValue::kBufferSize);
            value.size = rhsSize;
            value.capacity = StringValue::kBufferSize - 1;
            return;
        }
        auto& alloc = getAlloc();
        const size_type newCapacity =
            tiny_stl::min(rhsSize | StringValue::kBufferMask, max_size());
        pointer newPtr = alloc.allocate(newCapacity + 1);
        value.data.ptr = newPtr;
        Traits::move(newPtr, rhsPtr, rhsSize + 1);
        value.size = rhsSize;
        value.capacity = newCapacity;
    }

    void constructMove(basic_string& rhs) noexcept {
        auto& value = getVal();
        auto& rhsValue = rhs.getVal();
        if (rhsValue.isShortString()) {
            // copy short string
            Traits::move(value.data.buf, rhsValue.data.buf, rhsValue.size + 1);
        } else {
            value.data.ptr = rhsValue.data.ptr;
            rhsValue.data.ptr = pointer();
        }

        value.size = rhsValue.size;
        value.capacity = rhsValue.capacity;
        rhs.initEmpty();
    }

public:
    allocator_type get_allocator() const noexcept {
        return static_cast<allocator_type>(getAlloc());
    }

private:
    void initEmpty() noexcept {
        getVal().size = 0;
        getVal().capacity = StringValue::kBufferSize - 1;
        Traits::assign(getVal().data.buf[0], value_type());
    }

    basic_string& init(size_type count, value_type ch) {
        if (count <= getVal().capacity) {
            value_type* const ptr = getVal().getPtr();
            getVal().size = count;
            Traits::assign(ptr, count, ch);
            Traits::assign(ptr[count], value_type());

            return *this;
        }

        // allocate and assign
        return reallocAndAssign(
            count,
            [](value_type* const dst, size_type count, value_type ch) {
                Traits::assign(dst, count, ch);
                Traits::assign(dst[count], value_type());
            },
            ch);
    }

    basic_string& init(const basic_string& rhs, size_type pos,
                       size_type count = npos) {
        rhs.checkOffset(pos);
        count = tiny_stl::min(count, rhs.getVal().size - pos);
        return init(rhs.getVal().getPtr(), count);
    }

    basic_string& init(const value_type* str, size_type count) {
        if (count <= getVal().capacity) {
            value_type* const ptr = getVal().getPtr();
            getVal().size = count;
            Traits::move(ptr, str, count);
            Traits::assign(ptr[count], value_type());
            return *this;
        }

        // allocate and assign
        return reallocAndAssign(
            count,
            [](value_type* const dst, size_type count, const value_type* src) {
                Traits::move(dst, src, count);
                Traits::assign(dst[count], value_type());
            },
            str);
    }

    basic_string& init(const value_type* str) {
        return init(str, Traits::length(str));
    }

    template <typename Iter>
    void constructRange(Iter first, Iter last, input_iterator_tag) {
        TidyRAII<basic_string> guard{this};

        for (; first != last; ++first) {
            push_back(*first);
        }

        guard.obj = nullptr;
    }

    template <typename Iter>
    void constructRange(Iter first, Iter last, forward_iterator_tag) {
        const size_type count =
            static_cast<size_type>(tiny_stl::distance(first, last));
        reserve(count);
        constructRange(first, last, input_iterator_tag{});
    }

    void constructRange(const value_type* const first,
                        const value_type* const last,
                        random_access_iterator_tag) {
        if (first == last) {
            return;
        }

        init(first, static_cast<size_type>(last - first));
    }

    void constructRange(value_type* const first, value_type* const last,
                        random_access_iterator_tag) {
        if (first == last) {
            return;
        }

        init(first, static_cast<size_type>(last - first));
    }

    template <typename F, typename... Args>
    basic_string& reallocAndAssign(size_type newSize, F func, Args... args) {
        checkLength(newSize);
        Alloc& alloc = getAlloc();
        const size_type oldCapacity = getVal().capacity;
        const size_type newCapacity = capacityGrowth(newSize);

        pointer newPtr = alloc.allocate(newCapacity + 1); // for null character
        getVal().size = newSize;
        getVal().capacity = newCapacity;
        func(newPtr, newSize, args...);

        if (oldCapacity >= StringValue::kBufferSize) {
            alloc.deallocate(getVal().data.ptr, oldCapacity + 1);
        }
        getVal().data.ptr = newPtr;

        return *this;
    }

    template <typename F, typename... Args>
    basic_string& reallocAndAssignGrowBy(size_type growSize, F func,
                                         Args... args) {
        auto& value = getVal();
        const size_type oldSize = value.size;
        // check length
        if (max_size() - oldSize < growSize) {
            xLength();
        }

        const size_type newSize = oldSize + growSize;
        const size_type oldCapacity = value.capacity;
        const size_type newCapacity = capacityGrowth(newSize);
        auto& alloc = getAlloc();
        pointer newPtr = alloc.allocate(newCapacity + 1); // throws
        value.size = newSize;
        value.capacity = newCapacity;
        if (oldCapacity >= StringValue::kBufferSize) {
            pointer oldPtr = value.data.ptr;
            func(newPtr, oldPtr, oldSize, args...);
            alloc.deallocate(oldPtr, oldCapacity + 1);
        } else {
            func(newPtr, value.data.buf, oldSize, args...);
        }
        value.data.ptr = newPtr;
        return *this;
    }

    Alloc& getAlloc() noexcept {
        return allocVal.get_first();
    }

    const Alloc& getAlloc() const noexcept {
        return allocVal.get_first();
    }

    StringValue& getVal() noexcept {
        return allocVal.get_second();
    }

    const StringValue& getVal() const noexcept {
        return allocVal.get_second();
    }

    void tidy() noexcept {
        if (!getVal().isShortString()) {
            Alloc& alloc = getAlloc();
            const pointer ptr = getVal().getPtr();
            alloc.deallocate(ptr, getVal().capacity + 1);
        }
        initEmpty();
    }

public:
    reference at(size_type pos) {
        checkOffset(pos);
        return operator[](pos);
    }

    const_reference at(size_type pos) const {
        checkLength(pos);
        return operator[](pos);
    }

    reference operator[](size_type pos) {
        return getVal().getPtr()[pos];
    }

    const_reference operator[](size_type pos) const {
        return getVal().getPtr()[pos];
    }

    value_type& front() {
        return operator[](0);
    }

    const value_type& front() const {
        return operator[](0);
    }

    value_type& back() {
        return operator[](size() - 1);
    }

    const value_type& back() const {
        return operator[](size() - 1);
    }

    const value_type* data() const noexcept {
        return getVal().getPtr();
    }

    value_type* data() noexcept {
        return getVal().getPtr();
    }

    const value_type* c_str() const noexcept {
        return data();
    }

public:
    iterator begin() noexcept {
        return iterator(getVal().getPtr());
    }

    const_iterator begin() const noexcept {
        return const_iterator(getVal().getPtr());
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return iterator(getVal().getPtr() +
                        static_cast<difference_type>(getVal().size));
    }

    const_iterator end() const noexcept {
        const value_type* ptr =
            getVal().getPtr() + static_cast<difference_type>(getVal().size);
        return const_iterator{ptr};
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

public:
    bool empty() const noexcept {
        return size() == 0;
    }

    size_type size() const noexcept {
        return allocVal.get_second().size;
    }

    size_type length() const noexcept {
        return size();
    }

    size_type max_size() const noexcept {
        return tiny_stl::min(static_cast<size_type>(-1) / sizeof(value_type) -
                                 1,
                             static_cast<size_type>(
                                 std::numeric_limits<difference_type>::max()));
    }

    void reserve(size_type newCapacity = 0) {
        if (newCapacity < getVal().size) {
            shrink_to_fit();
            return;
        }

        if (newCapacity <= getVal().capacity) {
            return; // do nothing
        }

        // reallocate memory if newCapacity > oldCapacity
        const size_type oldSize = getVal().size;
        reallocAndAssignGrowBy(newCapacity - oldSize,
                               [](value_type* newPtr, const value_type* oldPtr,
                                  const size_type oldSizeX) {
                                   Traits::move(newPtr, oldPtr, oldSizeX + 1);
                               });
        getVal().size = oldSize;
    }

    size_type capacity() const noexcept {
        return allocVal.get_second().capacity;
    }

    void shrink_to_fit() {
        // do nothing
    }

private:
    void swapShortWithLong(StringValue& shortVal,
                           StringValue& longVal) noexcept {
        pointer ptr = longVal.data.ptr;
        Traits::move(longVal.data.buf, shortVal.data.buf,
                     StringValue::kBufferSize);
        shortVal.data.ptr = ptr;
    }

    void swapAux(basic_string& rhs) noexcept {
        auto& lhsVal = getVal();
        auto& rhsVal = rhs.getVal();
        const bool isShortLhs = lhsVal.isShortString();
        const bool isShortRhs = rhsVal.isShortString();

        if (isShortLhs) {
            if (isShortRhs)
                swapADL(lhsVal.data.ptr, rhsVal.data.ptr);
            else
                swapShortWithLong(lhsVal, rhsVal);
        } else { // lhs is long string
            if (isShortRhs) {
                swapShortWithLong(rhsVal, lhsVal);
            } else {
                value_type tmpBuf[StringValue::kBufferSize];
                Traits::move(tmpBuf, lhsVal.data.buf, StringValue::kBufferSize);
                Traits::move(lhsVal.data.buf, rhsVal.data.buf,
                             StringValue::kBufferSize);
                Traits::move(rhsVal.data.buf, tmpBuf, StringValue::kBufferSize);
            }
        }

        swapADL(lhsVal.size, rhsVal.size);
        swapADL(lhsVal.capacity, rhsVal.capacity);
    }

public:
    void clear() noexcept {
        tidy();
    }

    basic_string& insert(size_type pos, size_type count, value_type ch) {
        checkOffset(pos);
        const size_type oldCapcity = capacity();
        const size_type oldSize = size();
        if (count <= oldCapcity && oldSize <= oldCapcity - count) {
            auto& val = getVal();
            val.size += count;
            Traits::move(val.getPtr() + pos + count, val.getPtr() + pos,
                         oldSize - pos + 1);
            Traits::assign(val.getPtr() + pos, count, ch);
            Traits::assign(val.getPtr()[size()], value_type());
            return *this;
        }

        return reallocAndAssignGrowBy(
            count,
            [](value_type* newPtr, const value_type* oldPtr,
               const size_type xOldSize, const size_type xPos,
               const size_type xCount, const value_type xCh) {
                // const size_type newSize = xOldSize + xCount;
                Traits::move(newPtr, oldPtr, xPos);
                Traits::assign(newPtr + xPos, xCount, xCh);
                Traits::move(newPtr + xPos + xCount, oldPtr + xPos,
                             xOldSize - xPos + 1);
            },
            pos, count, ch);
    }

    basic_string& insert(size_type pos, const value_type* str) {
        return insert(pos, str, Traits::length(str));
    }

    basic_string& insert(size_type pos, const value_type* str,
                         size_type count) {
        checkOffset(pos);
        const size_type oldCapacity = capacity();
        const size_type oldSize = size();
        if (count <= oldCapacity && oldSize <= oldCapacity - count) {
            auto& val = getVal();
            val.size += count;
            Traits::move(val.getPtr() + pos + count, val.getPtr() + pos,
                         oldSize - pos + 1);
            Traits::move(val.getPtr() + pos, str, count);
            return *this;
        }

        return reallocAndAssignGrowBy(
            count,
            [](value_type* newPtr, const value_type* oldPtr,
               const size_type xOldSize, const size_type xPos,
               const value_type* xStr, const size_type xCount) {
                Traits::move(newPtr, oldPtr, xPos);
                Traits::move(newPtr + xPos, xStr, xCount);
                Traits::move(newPtr + xPos + xCount, oldPtr + xPos,
                             xOldSize - xPos + 1);
            },
            pos, str, count);
    }

    basic_string& insert(size_type pos, const basic_string& str) {
        return insert(pos, str.c_str(), str.size());
    }

    basic_string& insert(size_type pos, const basic_string& str,
                         size_type strPos, size_type count = npos) {
        str.checkOffset(strPos);
        auto& strVal = str.getVal();
        count = tiny_stl::min(count, str.size() - strPos);
        return insert(pos, str.getVal().getPtr() + strPos, count);
    }

    iterator insert(const_iterator pos, value_type ch) {
        const size_type offset = static_cast<size_type>(pos - cbegin());
        insert(offset, 1, ch);
        return begin() + static_cast<difference_type>(offset);
    }

    iterator insert(const_iterator pos, size_type count, value_type ch) {
        const size_type offset = static_cast<size_type>(pos - cbegin());
        insert(offset, count, ch);
        return begin() + static_cast<difference_type>(offset);
    }

    template <typename InIter, typename = enable_if_t<is_iterator_v<InIter>>>
    iterator insert(const_iterator pos, InIter first, InIter last) {
        const size_type offset = static_cast<size_type>(pos - cbegin());
        basic_string rhs{first, last};
        insert(offset, rhs);
        return begin() + static_cast<difference_type>(offset);
    }

    iterator insert(const_iterator pos,
                    std::initializer_list<value_type> ilist) {
        const size_type offset = static_cast<size_type>(pos - cbegin());
        insert(offset, ilist.begin(), ilist.size());
        return begin() + static_cast<difference_type>(offset);
    }

    basic_string& erase(size_type pos = 0, size_type count = npos) {
        checkOffset(pos);
        count = tiny_stl::min(count, size() - pos);
        auto& val = getVal();
        val.size -= count;
        Traits::move(val.getPtr() + pos, val.getPtr() + pos + count,
                     val.size - pos + 1 /*'\0'*/);

        return *this;
    }

    iterator erase(const_iterator pos) {
        const size_type offset = pos - cbegin();
        checkOffset(offset);
        erase(offset, 1);
        return iterator(data() + static_cast<difference_type>(offset));
    }

    iterator erase(const_iterator first, const_iterator last) {
        const size_type offset = first - cbegin();
        checkOffset(offset);
        const size_type count = last - first;
        erase(offset, count);
        return iterator(data() + static_cast<difference_type>(offset));
    }

    void push_back(value_type ch) {
        const size_type oldCapacity = capacity();
        const size_type oldSize = size();
        if (oldSize < oldCapacity) { // has enough space
            auto& val = getVal();
            ++val.size;
            pointer ptr = val.getPtr();
            Traits::assign(ptr[oldSize], ch);
            Traits::assign(ptr[oldSize + 1], value_type());
            return;
        }

        // reallocate and assign
        reallocAndAssignGrowBy(
            1,
            [](value_type* newPtr, const value_type* oldPtr,
               const size_type xOldSize, const value_type ch) {
                Traits::move(newPtr, oldPtr, xOldSize);
                Traits::assign(newPtr[xOldSize], ch);
                Traits::assign(newPtr[xOldSize + 1], value_type());
            },
            ch);
    }

    void pop_back() {
        if (empty())
            return;

        auto& val = getVal();
        --val.size;
        Traits::assign(val.getPtr()[size()], value_type());
    }

    void swap(basic_string& rhs) noexcept(
        allocator_traits<Alloc>::propagate_on_container_swap::value ||
        allocator_traits<Alloc>::is_always_equal::value) {
        assert(getAlloc() == rhs.getAlloc());
        if (this != tiny_stl::addressof(rhs)) {
            swapAux(rhs);
        }
    }

    basic_string& append(size_type count, value_type ch) {
        const size_type oldCapacity = capacity();
        const size_type oldSize = size();
        if (count <= oldCapacity &&
            oldSize <= oldCapacity - count) { // has enough space
            auto& val = getVal();
            val.size += count;
            Traits::assign(val.getPtr() + oldSize, count, ch);
            Traits::assign(val.getPtr()[oldSize + count], value_type());
            return *this;
        }

        return reallocAndAssignGrowBy(
            count,
            [](value_type* newPtr, const value_type* oldPtr,
               const size_type xOldSize, const size_type xCount,
               const value_type xCh) {
                Traits::move(newPtr, oldPtr, xOldSize);
                Traits::assign(newPtr + xOldSize, xCount, xCh);
                Traits::assign(newPtr[xOldSize + xCount], value_type());
            },
            count, ch);
    }

    basic_string& append(const basic_string& str) {
        return append(str.getVal().getPtr(), str.size());
    }

    basic_string& append(const basic_string& str, size_type pos,
                         size_type count = npos) {
        str.checkOffset(pos);
        count = tiny_stl::min(count, str.size() - pos);
        return append(str.getVal().getPtr(), count);
    }

    basic_string& append(const value_type* str, size_type count) {
        const size_type oldCapacity = capacity();
        const size_type oldSize = size();
        if (count <= oldCapacity && oldSize <= oldCapacity - count) {
            auto& val = getVal();
            val.size += count;
            Traits::move(val.getPtr() + oldSize, str, count);
            Traits::assign(val.getPtr()[oldSize + count], value_type());

            return *this;
        }

        return reallocAndAssignGrowBy(
            count,
            [](value_type* newPtr, const value_type* oldPtr,
               const size_type xOldSize, const value_type* xStr,
               const size_type xCount) {
                Traits::move(newPtr, oldPtr, xOldSize);
                Traits::move(newPtr + xOldSize, xStr, xCount);
                Traits::assign(newPtr[xOldSize + 1], value_type());
            },
            str, count);
    }

    basic_string& append(const value_type* str) {
        return append(str, Traits::length(str));
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    basic_string& append(InIter first, InIter last) {
        basic_string str{first, last};
        return append(str);
    }

    basic_string& append(std::initializer_list<value_type> ilist) {
        return append(ilist.begin(), ilist.size());
    }

    basic_string& operator+=(const basic_string& str) {
        return append(str);
    }

    basic_string& operator+=(value_type ch) {
        push_back(ch);
        return *this;
    }

    basic_string& operator+=(const value_type* str) {
        return append(str);
    }

    basic_string& operator+=(std::initializer_list<value_type> ilist) {
        return append(ilist.begin(), ilist.size());
    }

    int compare(const basic_string& rhs) const noexcept {
        const size_type lhsSize = size();
        const size_type rhsSize = rhs.size();
        int result = Traits::compare(data(), rhs.data(),
                                     tiny_stl::min(lhsSize, rhsSize));
        if (result != 0)
            return result;

        // assert result == 0
        if (lhsSize < rhsSize)
            return -1;
        if (lhsSize > rhsSize)
            return 1;

        // assert lhsSize == rhsSize
        return 0;
    }

    int compare(size_type pos1, size_type count1,
                const basic_string& rhs) const {
        basic_string lhs = substr(pos1, count1);
        return lhs.compare(rhs);
    }

    int compare(size_type pos1, size_type count1, const basic_string& rhs,
                size_type pos2, size_type count2 = npos) const {
        basic_string lhs = substr(pos1, count1);
        basic_string xrhs = rhs.substr(pos2, count2);
        return lhs.compare(xrhs);
    }

    int compare(const value_type* str) const {
        basic_string rhs{str};
        return compare(rhs);
    }

    int compare(size_type pos1, size_type count1, const value_type* str) const {
        basic_string lhs = substr(pos1, count1);
        return lhs.compare(basic_string{str});
    }

    int compare(size_type pos1, size_type count1, const value_type* str,
                size_type count2) const {
        basic_string lhs = substr(pos1, count1);
        basic_string rhs{str, count2};
        return lhs.compare(rhs);
    }

    bool starts_with(value_type ch) const noexcept {
        return size() >= 1 && this->front() == ch;
    }

    bool starts_with(const value_type* str) const {
        const size_type strLen = Traits::length(str);
        return size() >= strLen && compare(0, strLen, str) == 0;
    }

    bool ends_with(value_type ch) const noexcept {
        return size() >= 1 && this->back() == ch;
    }

    bool ends_with(const value_type* str) const {
        const size_type lhsSize = size();
        const size_type strLen = Traits::length(str);
        return lhsSize >= strLen && compare(lhsSize - strLen, strLen, str) == 0;
    }

    basic_string& replace(size_type pos, size_type count,
                          const basic_string& str) {
        return replace(pos, count, str.c_str(), str.size());
    }

    basic_string& replace(const_iterator first, const_iterator last,
                          const basic_string& str) {
        return replace(first - begin(), last - first, str.c_str(), str.size());
    }

    basic_string& replace(size_type pos, size_type count,
                          const basic_string& str, size_type pos2,
                          size_type count2 = npos) {
        return replace(pos, count, str.substr(pos2, count2));
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    basic_string& replace(const_iterator first, const_iterator last,
                          InIter first2, InIter last2) {
        return replace(first - begin(), last - first,
                       basic_string{first2, last2});
    }

    basic_string& replace(size_type pos, size_type count, const value_type* str,
                          size_type count2) {
        checkOffset(pos);
        auto& val = getVal();
        const size_type oldSize = size();
        count = tiny_stl::min(count, oldSize - pos);
        if (count == count2) {
            Traits::move(val.getPtr() + pos, str, count);
            return *this;
        }

        const size_type suffixSize = oldSize - pos - count + 1;
        if (count > count2) {
            val.size = oldSize - (count - count2);
            value_type* oldPtr = val.getPtr();
            value_type* replaceAt = oldPtr + pos;
            Traits::move(replaceAt, str, count2);
            Traits::move(replaceAt + count2, replaceAt + count, suffixSize);
            return *this;
        }

        // count2 > count
        const size_type growSize = count2 - count;
        const size_type oldCapacity = capacity();
        if (growSize < oldCapacity - oldSize) {
            val.size = oldSize + growSize;
            value_type* replaceAt = val.getPtr() + pos;
            value_type* oldSuffixAt = replaceAt + count;
            value_type* newSuffixAt = oldSuffixAt + growSize;

            Traits::move(newSuffixAt, oldSuffixAt, suffixSize);
            Traits::move(replaceAt, str, count2);
            return *this;
        }

        return reallocAndAssignGrowBy(
            growSize,
            [](value_type* newPtr, const value_type* oldPtr,
               const size_type oldSize, const size_type offset,
               const size_type xCount1, const value_type* xStr,
               const size_type xCount2) {
                Traits::move(newPtr, oldPtr, offset);
                Traits::move(newPtr + offset, xStr, xCount2);
                Traits::move(newPtr + offset + xCount2,
                             oldPtr + offset + xCount1,
                             oldSize - offset - xCount1 + 1);
            },
            pos, count, str, count2);
    }

    basic_string& replace(const_iterator first, const_iterator last,
                          const value_type* str, size_type count2) {
        return replace(first - begin(), last - first, str, count2);
    }

    basic_string& replace(size_type pos, size_type count,
                          const value_type* str) {
        return replace(pos, count, str, Traits::length(str));
    }

    basic_string& replace(const_iterator first, const_iterator last,
                          const value_type* str) {
        return replace(first - begin(), last - first, str);
    }

    basic_string& replace(size_type pos, size_type count, size_type count2,
                          value_type ch) {
        checkOffset(pos);
        auto& val = getVal();
        const size_type oldSize = size();
        count = tiny_stl::min(count, oldSize - pos);
        if (count == count2) {
            Traits::assign(val.getPtr() + pos, count, ch);
            return *this;
        }

        const size_type oldCapacity = capacity();
        if (count2 < count || count2 - count < oldCapacity - oldSize) {
            val.size = oldSize + count2 - count;
            value_type* oldPtr = val.getPtr();
            value_type* replaceAt = oldPtr + pos;
            Traits::move(replaceAt + count2, replaceAt + count,
                         oldSize - pos - count + 1);
            Traits::assign(replaceAt, count2, ch);
            return *this;
        }

        return reallocAndAssignGrowBy(
            count2 - count,
            [](value_type* newPtr, const value_type* oldPtr,
               const size_type oldSize, const size_type offset,
               const size_type xCount, const size_type xCount2, value_type ch) {
                Traits::move(newPtr, oldPtr, offset);
                Traits::assign(newPtr + offset, xCount2, ch);
                Traits::move(newPtr + offset + xCount2,
                             oldPtr + offset + xCount,
                             oldSize - xCount - offset + 1);
            },
            pos, count, count2, ch);
    }

    basic_string& replace(const_iterator first, const_iterator last,
                          size_type count2, value_type ch) {
        return replace(first - begin(), last - first, count2, ch);
    }

    basic_string& replace(const_iterator first, const_iterator last,
                          std::initializer_list<value_type> ilist) {
        return replace(first - begin(), last - first, ilist.begin(),
                       ilist.size());
    }

    size_type copy(value_type* dst, size_type count, size_type pos) const {
        checkOffset(pos);
        count = tiny_stl::min(count, size() - pos);
        Traits::move(dst, data() + pos, count);

        return count;
    }

    void resize(size_type newSize) {
        return resize(newSize, value_type());
    }

    void resize(size_type newSize, value_type ch) {
        const size_type oldSize = size();
        if (oldSize == newSize)
            return;
        if (oldSize < newSize) {
            append(newSize - oldSize, ch);
        } else { // oldSize > newSize
            erase(newSize);
        }
    }

    basic_string substr(size_type pos = 0, size_type count = npos) const {
        return basic_string{*this, pos, count};
    }

private:
    size_type findHelper(const value_type* str, size_type pos,
                         size_type count) const noexcept {
        // native find algorithm
        size_type lhsSize = size();
        if (count > lhsSize || pos > lhsSize - count)
            return npos;
        if (count == 0)
            return pos;

        for (size_type i = pos; i <= lhsSize - count; ++i) {
            // mismatched the first character
            if (!Traits::eq(data()[i], str[0]))
                continue;

            size_type j = 1;
            for (; j < count; ++j) {
                if (!Traits::eq(data()[i + j], str[j]))
                    break;
            }
            if (j == count)
                return i;
        }

        return npos;
    }

    // [0, pos]
    size_type rfindHelper(const value_type* str, size_type pos,
                          size_type count) const noexcept {
        // native find algorithm
        size_type lhsSize = size();
        if (count > lhsSize || pos < count)
            return npos;
        if (lhsSize == 0)
            return npos;
        if (count == 0)
            return pos;

        pos = tiny_stl::min(pos, lhsSize - 1);
        // pos >= count
        for (difference_type i = static_cast<difference_type>(pos); i >= 0;
             --i) {
            // mismatched the first character
            if (!Traits::eq(data()[i], str[0]))
                continue;

            size_type j = 1;
            for (; j < count; ++j) {
                if (!Traits::eq(data()[i + j], str[j]))
                    break;
            }
            if (j == count)
                return i;
        }
        return npos;
    }

public:
    size_type find(const basic_string& str, size_type pos = 0) const noexcept {
        return findHelper(str.c_str(), pos, str.size());
    }

    size_type find(const value_type* str, size_type pos,
                   size_type count) const {
        return findHelper(str, pos, count);
    }

    size_type find(const value_type* str, size_type pos = 0) const {
        return findHelper(str, pos, Traits::length(str));
    }

    size_type find(value_type ch, size_type pos = 0) const noexcept {
        const value_type* findAt = Traits::find(data() + pos, size() - pos, ch);
        return findAt == nullptr ? npos
                                 : static_cast<size_type>(findAt - data());
    }

    size_type rfind(const basic_string& str,
                    size_type pos = npos) const noexcept {
        return rfindHelper(str.data(), pos, str.size());
    }

    size_type rfind(const value_type* str, size_type pos,
                    size_type count) const {
        return rfindHelper(str, pos, count);
    }

    size_type rfind(value_type ch, size_type pos = npos) const noexcept {
        pos = tiny_stl::min(pos, size() - 1);
        difference_type i = pos;
        for (; i >= 0; --i) {
            if (data()[i] == ch)
                return static_cast<size_type>(i);
        }

        return npos;
    }

private:
    void checkLength(size_type newSize) const {
        if (newSize >= max_size()) {
            xLength();
        }
    }

    void checkOffset(size_type offset) const {
        if (offset > size()) {
            xRange();
        }
    }

    size_type capacityGrowth(size_type newSize) const {
        const size_type oldSize = allocVal.get_second().size;
        const size_type masked = newSize | StringValue::kBufferMask;
        const size_type maxSize = max_size();
        if (masked > maxSize) {
            return maxSize;
        }

        if (oldSize > maxSize - oldSize / 2) { // for avoiding overflow
            return maxSize;
        }

        return tiny_stl::max(masked, oldSize + oldSize / 2);
    }

private:
    [[noreturn]] static void xLength() {
        throw "tiny_stl::basic_string<CharT> too long";
    }

    [[noreturn]] static void xRange() {
        throw "invalid tiny_stl::basic_string<CharT> index";
    }
};

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc>
operator+(const basic_string<CharT, Traits, Alloc>& lhs,
          const basic_string<CharT, Traits, Alloc>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(lhs.size() + rhs.size());
    tmp += lhs;
    tmp += rhs;
    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc>
operator+(const basic_string<CharT, Traits, Alloc>& lhs, const CharT* rhs) {
    basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(lhs.size() + Traits::length(rhs));
    tmp += lhs;
    tmp += rhs;
    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc>
operator+(const basic_string<CharT, Traits, Alloc>& lhs, CharT rhs) {
    basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(lhs.size() + 1);
    tmp += lhs;
    tmp += rhs;
    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc>
operator+(const CharT* lhs, const basic_string<CharT, Traits, Alloc>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(Traits::length(lhs) + rhs.size());
    tmp += lhs;
    tmp += rhs;
    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc>
operator+(CharT lhs, const basic_string<CharT, Traits, Alloc>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(1 + rhs.size());
    tmp += lhs;
    tmp += rhs;
    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator==(const basic_string<CharT, Traits, Alloc>& lhs,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return lhs.size() == rhs.size() &&
           tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator!=(const basic_string<CharT, Traits, Alloc>& lhs,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<(const basic_string<CharT, Traits, Alloc>& lhs,
                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>(const basic_string<CharT, Traits, Alloc>& lhs,
                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return rhs < lhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<=(const basic_string<CharT, Traits, Alloc>& lhs,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(rhs < lhs);
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>=(const basic_string<CharT, Traits, Alloc>& lhs,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs < rhs);
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator==(const CharT* lstr,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> lhs{lstr};
    return lhs == rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator==(const basic_string<CharT, Traits, Alloc>& lhs,
                       const CharT* rstr) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> rhs{rstr};
    return lhs == rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator!=(const CharT* lstr,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> lhs{lstr};
    return lhs != rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator!=(const basic_string<CharT, Traits, Alloc>& lhs,
                       const CharT* rstr) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> rhs{rstr};
    return lhs != rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<(const CharT* lstr,
                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> lhs{lstr};
    return lhs < rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<(const basic_string<CharT, Traits, Alloc>& lhs,
                      const CharT* rstr) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> rhs{rstr};
    return lhs < rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>(const CharT* lstr,
                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> lhs{lstr};
    return lhs > rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>(const basic_string<CharT, Traits, Alloc>& lhs,
                      const CharT* rstr) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> rhs{rstr};
    return lhs > rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<=(const CharT* lstr,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> lhs{lstr};
    return lhs <= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<=(const basic_string<CharT, Traits, Alloc>& lhs,
                       const CharT* rstr) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> rhs{rstr};
    return lhs <= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>=(const CharT* lstr,
                       const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> lhs{lstr};
    return lhs >= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>=(const basic_string<CharT, Traits, Alloc>& lhs,
                       const CharT* rstr) noexcept {
    tiny_stl::basic_string<CharT, Traits, Alloc> rhs{rstr};
    return lhs >= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
void swap(
    basic_string<CharT, Traits, Alloc>& lhs,
    basic_string<CharT, Traits, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename CharT, typename Traits, typename Alloc>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const basic_string<CharT, Traits, Alloc>& str) {
    // no format output
    os << str.c_str();
    return os;
}

template <typename CharT, typename Traits, typename Alloc>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is,
           basic_string<CharT, Traits, Alloc>& str) {
    // no format input
    is >> str.data();

    return is;
}

template <typename CharT, typename Traits, typename Alloc>
struct hash<basic_string<CharT, Traits, Alloc>> {
    using argument_type = basic_string<CharT, Traits, Alloc>;
    using result_type = std::size_t;

    std::size_t
    operator()(const basic_string<CharT, Traits, Alloc>& str) const noexcept {
        return tiny_stl::hashFNV(str.c_str(), str.size());
    }
};

namespace {

template <typename CharT, typename T>
inline basic_string<CharT> IntegerToStringAux(T val, const CharT* zero) {
    bool isNegative = val < 0;
    CharT buffer[21]; // -2^63 ~ 2^64-1
    CharT* ptr = buffer;
    do {
        int lp = static_cast<int>(val % 10);
        val /= 10;
        *ptr++ = zero[lp];
    } while (val);

    if (isNegative) {
        *ptr++ = basic_string<CharT>::traits_type::to_char_type('-');
    }
    *ptr = basic_string<CharT>::traits_type::to_char_type('\0');
    reverse(buffer, ptr);
    return basic_string<CharT>{buffer};
}

template <typename CharT, typename T>
inline basic_string<CharT> IntegerToStringHelp(T val, true_type) {
    static const char digits[] = "9876543210123456789";
    static const char* zero = digits + 9;
    return IntegerToStringAux(val, zero);
}

template <typename CharT, typename T>
inline basic_string<CharT> IntegerToStringHelp(T val, false_type) {
    static const wchar_t digits[] = L"9876543210123456789";
    static const wchar_t* zero = digits + 9;
    return IntegerToStringAux(val, zero);
}

template <
    typename CharT, typename T,
    enable_if_t<is_same_v<CharT, char> || is_same_v<CharT, wchar_t>, int> = 0>
inline basic_string<CharT> IntegerToString(T val) {
    static_assert(is_integral_v<T>, "T must be integral");
    static_assert(is_same_v<CharT, char> || is_same_v<CharT, wchar_t>,
                  "CharT must be char or wchar_t");
    return IntegerToStringHelp<CharT>(val, is_same<CharT, char>());
}

} // namespace

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;

inline string to_string(int value) {
    return IntegerToString<char>(value);
}

inline string to_string(long value) {
    return IntegerToString<char>(value);
}

inline string to_string(unsigned value) {
    return IntegerToString<char>(value);
}

inline string to_string(unsigned long value) {
    return IntegerToString<char>(value);
}

inline string to_string(long long value) {
    return IntegerToString<char>(value);
}

inline string to_string(unsigned long long value) {
    return IntegerToString<char>(value);
}

inline wstring to_wstring(int value) {
    return IntegerToString<wchar_t>(value);
}

inline wstring to_wstring(long value) {
    return IntegerToString<wchar_t>(value);
}

inline wstring to_wstring(unsigned value) {
    return IntegerToString<wchar_t>(value);
}

inline wstring to_wstring(unsigned long value) {
    return IntegerToString<wchar_t>(value);
}

inline wstring to_wstring(long long value) {
    return IntegerToString<wchar_t>(value);
}

inline wstring to_wstring(unsigned long long value) {
    return IntegerToString<wchar_t>(value);
}

#ifdef TINY_STL_CXX14

namespace literals {

namespace string_literals {

inline string operator""_s(const char* str, std::size_t len) {
    return string{str, len};
}

inline wstring operator""_s(const wchar_t* str, std::size_t len) {
    return wstring{str, len};
}

inline u16string operator""_s(const char16_t* str, std::size_t len) {
    return u16string{str, len};
}

inline u32string operator""_s(const char32_t* str, std::size_t len) {
    return u32string{str, len};
}

} // namespace string_literals
} // namespace literals

#endif // TINY_STL_CXX14

} // namespace tiny_stl
