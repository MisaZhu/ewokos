// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <initializer_list>
#include <string>

#include "memory.hpp"

namespace tiny_stl {

template <typename T>
struct CowStringConstIterator {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;
    using difference_type = std::ptrdiff_t;
    using Self = CowStringConstIterator<T>;

    T* ptr;

    CowStringConstIterator() = default;
    CowStringConstIterator(T* p) : ptr(p) {
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
struct CowStringIterator : CowStringConstIterator<T> {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using difference_type = std::ptrdiff_t;
    using Base = CowStringConstIterator<T>;
    using Self = CowStringIterator<T>;

    CowStringIterator() = default;
    CowStringIterator(T* p) : Base(p) {
    }

    reference operator*() const {
        return *this->ptr;
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

    Self operator+(difference_type n) const {
        Self tmp = *this;
        return tmp += n;
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
}; // class StringIterator<T>

namespace extra {

// ref <<More Effective c++>>

// Reference count base class
// Provide interface
// Improved the original
class RCObject {
private:
    std::size_t mRefCount;

protected: // Derived class call
    RCObject() : mRefCount(0) {
    }
    RCObject(const RCObject& rhs) = default;
    RCObject& operator=(const RCObject& rhs) = default;
    virtual ~RCObject() {
    }

public:
    void retain() noexcept {
        ++mRefCount;
    }

    void release() noexcept {
        if (--mRefCount == 0)
            delete this;
    }

    bool isShared() const noexcept {
        return mRefCount > 1;
    }

    std::size_t getRefCount() const noexcept {
        return mRefCount;
    }
};

// like smart pointer
template <typename RCObj>
class RCPtr {
private:
    RCObj* pointee;
    void init() {
        if (pointee == nullptr)
            return;

        pointee->retain();
    }

public:
    RCPtr(RCObj* p = nullptr) : pointee(p) {
        init();
    }

    RCPtr(const RCPtr& rhs) : pointee(rhs.pointee) {
        init();
    }

    RCPtr(RCPtr&& rhs) noexcept : pointee(rhs.pointee) {
        rhs.pointee = nullptr;
    }

    ~RCPtr() noexcept {
        if (pointee != nullptr)
            pointee->release();
    }

    RCPtr& operator=(const RCPtr& rhs) {
        if (pointee != rhs.pointee) {
            if (pointee != nullptr)
                pointee->release();
            pointee = rhs.pointee;
            init();
        }

        return *this;
    }

    RCPtr& operator=(RCPtr&& rhs) noexcept {
        if (pointee != rhs.pointee) {
            if (pointee != nullptr)
                pointee->release();
            pointee = rhs.pointee;
            rhs.pointee = nullptr;
        }

        return *this;
    }

    RCObj* operator->() const {
        return pointee;
    }

    RCObj& operator*() const {
        return *pointee;
    }

}; // class RCPtr<T>

} // namespace extra

template <typename CharT, typename Traits = std::char_traits<CharT>,
          typename Alloc = allocator<CharT>>
class cow_basic_string {
public:
    static_assert(is_same<typename Traits::char_type, CharT>::value,
                  "char type error");

public:
    using traits_type = Traits;
    using value_type = CharT;
    using allocator_type = Alloc;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using reference = CharT&;
    using const_reference = const CharT&;
    using pointer = CharT*;
    using const_pointer = const CharT*;
    using iterator = CowStringIterator<CharT>;
    using const_iterator = CowStringConstIterator<CharT>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

    using AllocTraits = tiny_stl::allocator_traits<Alloc>;

public:
    static const size_type npos = static_cast<size_type>(-1);

private:
    // nested struct
    // manage resources
    struct StringValue : public extra::RCObject {
        size_type size;
        size_type capa;
        CharT* data;
        Alloc alloc;

        void init() {
            data = alloc.allocate(capa);
            data[0] = traits_type::to_char_type(0);
        }

        void init(const CharT* s) {
            data = alloc.allocate(capa);
            traits_type::move(data, s, size);
            data[size] = traits_type::to_char_type(0);
        }

        void init(size_type count, CharT ch) {
            data = alloc.allocate(capa);
            traits_type::assign(data, count, ch);
            data[size] = traits_type::to_char_type(0);
        }

        static const size_type kSmallestCapacity = 8;

        explicit StringValue() : size(0), capa(kSmallestCapacity), alloc() {
            init();
        }

        StringValue(const CharT* s)
            : size(traits_type::length(s)), capa(size < 8 ? 8 : size + 1),
              alloc() {
            init(s);
        }

        StringValue(const CharT* s, size_type count)
            : size(count), capa(size < 8 ? 8 : size + 1), alloc() {
            init(s);
        }

        StringValue(size_type count, CharT ch)
            : size(count), capa(size < 8 ? 8 : size + 1), alloc() {
            init(count, ch);
        }

        StringValue(const StringValue& rhs, size_type pos, size_type count)
            : size(count < rhs.size - pos ? count : rhs.size - pos),
              capa(size < 8 ? 8 : size + 1), alloc() {
            assert(pos < rhs.size);
            init(rhs.data + pos);
        }

        template <typename InIter,
                  typename = enable_if_t<is_iterator<InIter>::value>>
        StringValue(InIter first, InIter last)
            : size(tiny_stl::distance(first, last)),
              capa(size < 8 ? 8 : size + 1), alloc() {
            data = alloc.allocate(capa);
            size_type i;
            for (i = 0; first != last; ++first, ++i)
                data[i] = *first;

            data[size] = traits_type::to_char_type(0);
        }

        ~StringValue() {
            if (data != nullptr)
                alloc.deallocate(data, capa);
        }
    };

    // like smart pointer
    extra::RCPtr<StringValue> value;

public:
    // delete user allocator version
    // (1)
    cow_basic_string() : value(new StringValue()) {
    }

    // (2)
    cow_basic_string(size_type count, CharT ch)
        : value(new StringValue(count, ch)) {
    }

    // (3)
    cow_basic_string(const cow_basic_string& rhs, size_type pos,
                     size_type count = npos)
        : value(new StringValue(*rhs.value, pos, count)) {
    }

    // (4)
    cow_basic_string(const CharT* s, size_type count)
        : value(new StringValue(s, count)) {
    }

    // (5)
    cow_basic_string(const CharT* s) : value(new StringValue(s)) {
    }

    // (6)
    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    cow_basic_string(InIter first, InIter last)
        : value(new StringValue(first, last)) {
    }

    // (7)
    // call RCPtr::RCPtr(const RCPtr& rhs);
    // only add reference count
    cow_basic_string(const cow_basic_string& rhs) = default;

    // (8)
    cow_basic_string(cow_basic_string&& rhs) noexcept
        : value(tiny_stl::move(rhs.value)) {
    }

    // (9)
    cow_basic_string(std::initializer_list<CharT> ilist)
        : value(new StringValue(ilist.begin(), ilist.end() - ilist.begin())) {
    }

    allocator_type get_allocator() {
        return value.pointee->alloc;
    }

    cow_basic_string& operator=(const cow_basic_string& rhs) {
        assert(this != tiny_stl::addressof(rhs));
        value = rhs.value;

        return *this;
    }

    cow_basic_string& operator=(cow_basic_string&& rhs) noexcept {
        assert(this != tiny_stl::addressof(rhs));
        value = tiny_stl::move(rhs.value);

        return *this;
    }

    cow_basic_string& operator=(const CharT* s) {
        assert(s != nullptr);
        value = new StringValue(s);

        return *this;
    }

    cow_basic_string& operator=(CharT ch) {
        value = new StringValue(1, ch);

        return *this;
    }

    cow_basic_string& operator=(std::initializer_list<CharT>& ilist) {
        value = new StringValue(ilist.begin(), ilist.end() - ilist.begin());

        return *this;
    }

    cow_basic_string& assign(size_type count, CharT ch) {
        value = new StringValue(count, ch);

        return *this;
    }

    cow_basic_string& assign(const cow_basic_string& rhs) {
        assert(this != tiny_stl::addressof(rhs));

        value = rhs.value;
        return *this;
    }

    cow_basic_string& assign(const cow_basic_string& rhs, size_type pos,
                             size_type count = npos) {
        value = new StringValue(*rhs.value, pos, count);

        return *this;
    }

    cow_basic_string& assign(cow_basic_string&& rhs) {
        assert(this != tiny_stl::addressof(rhs));
        value = tiny_stl::move(rhs.value);

        return *this;
    }

    cow_basic_string& assign(const CharT* s, size_type count) {
        assert(s != nullptr);

        value = new StringValue(s, count);
        return *this;
    }

    cow_basic_string& assign(const CharT* s) {
        assert(s != nullptr);
        value = new StringValue(s);

        return *this;
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    cow_basic_string& assign(InIter first, InIter last) {
        value = new StringValue(first, last - first);

        return *this;
    }

    cow_basic_string& assign(std::initializer_list<CharT> ilist) {
        value = new StringValue(ilist.begin(), ilist.end() - ilist.begin());

        return *this;
    }

    reference at(size_type pos) {
#ifndef NDEBUG
        if (pos >= size())
            xRange();
#endif
        if (value->isShared())
            value = new StringValue(value->data); // copy when reference access

        return *(value->data + pos);
    }

    const_reference at(size_type pos) const {
#ifndef NDEBUG
        if (pos >= size())
            xRange();
#endif

        return *(value->data + pos);
    }

    reference operator[](size_type pos) {
        assert(pos <= size());

        if (value->isShared())
            value = new StringValue(value->data); // copy when reference access

        // ub: modify this->operator[size()]
        return *(value->data + pos);
    }

    const_reference operator[](size_type pos) const {
        assert(pos <= size());

        return *(value->data + pos);
    }

    reference front() {
        assert(!empty());

        return this->operator[](0);
    }

    const_reference front() const {
        assert(!empty());

        return this->operator[](0);
    }

    reference back() {
        assert(!empty());

        return this->operator[](size() - 1);
    }

    const_reference back() const {
        assert(!empty());

        return this->operator[](size() - 1);
    }

    pointer data() noexcept {
        if (value->isShared())
            value = new StringValue(value->data);

        return value->data;
    }

    const_pointer data() const noexcept {
        return value->data;
    }

    const_pointer c_str() const noexcept {
        return value->data;
    }

    iterator begin() noexcept {
        if (value->isShared())
            value = new StringValue(value->data);

        return iterator(value->data);
    }

    const_iterator begin() const noexcept {
        return const_iterator(value->data);
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        if (value->isShared())
            value = new StringValue(value->data);

        return iterator(value->data + value->size);
    }

    const_iterator end() const noexcept {
        return const_iterator(value->data + value->size);
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

    bool empty() const noexcept {
        return begin() == end();
    }

    size_type size() const noexcept {
        return value->size;
    }

    size_type length() const noexcept {
        return size();
    }

    size_type max_size() const noexcept {
        return npos >> 1;
    }

    size_type capacity() const noexcept {
        return value->capa;
    }

    void shrink_to_fit() noexcept {
        // do nothing, avoid too slow
    }

    void clear() noexcept {
        if (this->isShared())
            value = new StringValue(value->data);

        value->size = 0;
    }

private:
    void checkLength(size_type newSize) const {
        if (newSize >= max_size())
            xLength();
    }

    void checkRange(size_type pos, size_type len) const {
        if (pos > len)
            xRange();
    }

    void checkLengthAndRange(size_type newSize, size_type pos,
                             size_type len) const {
        checkLength(newSize);
        checkRange(pos, len);
    }

    template <typename Lambda>
    void resizeHelper(size_type count, Lambda lambda) {
        checkLength(count);

        if (this->isShared())
            value = new StringValue(value->data);

        if (count < size()) {
            value->size = count;
            value->data[value->size] = Traits::to_char_type(0);
        } else if (count > value->size) {
            if (count > value->capa) { // count > old capacity: reallocate
                CharT* newPtr = value->alloc.allocate(count + 1);
                Traits::move(newPtr, value->data, value->size);
                value->alloc.deallocate(value->data, value->capa);

                value->capa = count + 1;
                value->data = newPtr;
            }

            lambda();
        }
    }

public:
    void resize(size_type count, CharT ch) {
        auto lambda_fill = [this, count, ch]() {
            traits_type::assign(this->value->data + this->value->size,
                                count - this->value->size, ch);
            this->value->size = count;
            value->data[value->size] = traits_type::to_char_type(0);
        };
        resizeHelper(count, lambda_fill);
    }

    void resize(size_type count) {
        auto lambda_default = [this, count]() {
            this->value->size = count;
            this->value->data[this->value->size] = traits_type::to_char_type(0);
        };
        resizeHelper(count, lambda_default);
    }

    void reserve(size_type new_cap = 0) {
        if (this->isShared())
            value = new StringValue(value->data);

        checkLength(new_cap);

        size_type oldCapacity = capacity();
        if (new_cap > oldCapacity)
            reallocteHelper(value->size);

        // else do nothing
    }

private:
    size_type capacityGrowth(size_type newSize) const noexcept {
        const size_type oldCapacity = capacity();

        if ((oldCapacity << 1) > max_size())
            return newSize;

        const size_type newCapacity = (oldCapacity << 1);

        return newCapacity < newSize ? newSize : newCapacity;
    }

    void updatePointer(const pointer newFirst, size_type newCapacity) {
        if (value->data != nullptr) {
            value->alloc.deallocate(value->data, capacity());
        }

        value->data = newFirst;
        value->capa = newCapacity;
    }

    void reallocteHelper(size_type newSize) {
        size_type newCapacity = capacityGrowth(newSize);
        const pointer newFirst = value->alloc.allocate(newCapacity);
        traits_type::move(newFirst, value->data, value->size);
        updatePointer(newFirst, newCapacity);
    }

public:
    void push_back(CharT ch) {
        if (this->isShared())
            value = new StringValue(value->data);

        size_type oldCapacity = capacity();
        if (value->size == oldCapacity - 1) {
            size_type newSize = value->size + 1;

            reallocteHelper(newSize);
        }

        value->data[value->size++] = ch;
        value->data[value->size] = traits_type::to_char_type(0);
    }

    cow_basic_string& erase(size_type index = 0, size_type count = npos) {
        assert(index < size());
        if (this->isShared())
            value = new StringValue(value->data);

        size_type realCount = tiny_stl::min(size() - index, count);

        size_type newPos = index + realCount;

        traits_type::move(value->data + index, value->data + newPos,
                          size() - newPos + 1);
        value->size = size() - realCount;

        return *this;
    }

    iterator erase(const_iterator pos) {
        erase(pos.ptr - value->data, 1);

        return iterator(pos.ptr);
    }

    iterator erase(const_iterator first, const_iterator last) {
        erase(first.ptr - value->data, last - first);

        return iterator(first.ptr);
    }

    void pop_back() {
        erase(size() - 1, 1);
    }

public:
    cow_basic_string& append(size_type count, CharT ch) {
        size_type oldSize = size();
        size_type newSize = oldSize + count;

        checkLength(newSize);

        if (this->isShared())
            value = new StringValue(value->data);

        if (count >= value->capa - oldSize) // reallocate
            reallocteHelper(newSize);

        traits_type::assign(value->data + oldSize, count, ch);
        value->data[newSize] = traits_type::to_char_type(0);
        value->size = newSize;

        return *this;
    }

    cow_basic_string& append(const cow_basic_string& str) {
        const size_type count = str.size();

        size_type oldSize = size();
        size_type newSize = oldSize + count;

        checkLength(newSize);

        if (this->isShared())
            value = new StringValue(value->data);

        if (count >= value->capa - oldSize)
            reallocteHelper(newSize);

        traits_type::move(value->data + oldSize, str.c_str(), count);
        value->data[newSize] = traits_type::to_char_type(0);
        value->size = newSize;

        return *this;
    }

    cow_basic_string& append(const cow_basic_string& str, size_type pos,
                             size_type count = npos) {
        assert(pos <= str.size());

        return append(str.c_str() + pos, count);
    }

    cow_basic_string& append(const CharT* s, size_type count) {
        count = tiny_stl::min(count, traits_type::length(s));
        size_type oldSize = size();
        size_type newSize = oldSize + count;

        checkLength(newSize);

        if (this->isShared())
            value = new StringValue(value->data);

        if (count >= value->capa - oldSize)
            reallocteHelper(newSize);

        traits_type::move(value->data + oldSize, s, count);
        value->data[newSize] = traits_type::to_char_type(0);
        value->size = newSize;

        return *this;
    }

    cow_basic_string& append(const CharT* s) {
        return append(s, traits_type::length(s));
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    cow_basic_string& append(InIter first, InIter last) {
        cow_basic_string str{first, last};

        return append(str);
    }

    cow_basic_string& append(std::initializer_list<CharT> ilist) {
        cow_basic_string str{ilist};

        return append(str);
    }

    cow_basic_string& operator+=(const cow_basic_string& str) {
        return append(str);
    }

    cow_basic_string& operator+=(CharT ch) {
        return append(1, ch);
    }

    cow_basic_string& operator+=(const CharT* s) {
        return append(s);
    }

    cow_basic_string& operator+=(std::initializer_list<CharT> ilist) {
        return append(ilist);
    }

    cow_basic_string& insert(size_type pos, size_type count, CharT ch) {
        size_type oldSize = size();
        size_type newSize = oldSize + count;

        checkLengthAndRange(newSize, pos, oldSize);

        if (this->isShared())
            value = new StringValue(value->data);

        if (count < capacity() - oldSize) { // no reallocate
            CharT* oldPtr = value->data;
            CharT* insertPos = oldPtr + pos;
            traits_type::move(insertPos + count, insertPos, oldSize - pos + 1);
            traits_type::assign(insertPos, count, ch);
        } else { // reallocate
            size_type newCapacity = capacityGrowth(newSize);
            const pointer newFirst = value->alloc.allocate(newCapacity);
            traits_type::move(newFirst, value->data,
                              pos); // move elements before the pos
            traits_type::move(newFirst +
                                  (count + pos), // move elements after the pos
                              value->data + pos, oldSize - pos);
            updatePointer(newFirst, newCapacity);
            traits_type::assign(newFirst + pos, count,
                                ch); // fill count characters
        }

        value->data[newSize] = traits_type::to_char_type(0);
        value->size = newSize;

        return *this;
    }

    cow_basic_string& insert(size_type pos, const CharT* s, size_type count) {
        size_type oldSize = size();
        size_type newSize = oldSize + count;

        checkLengthAndRange(newSize, pos, oldSize);

        if (this->isShared())
            value = new StringValue(value->data);

        if (count < capacity() - oldSize) { // no reallocate
            CharT* oldPtr = value->data;
            CharT* insertPos = oldPtr + pos;
            traits_type::move(insertPos + count, insertPos, oldSize - pos + 1);
            traits_type::move(insertPos, s, count);
        } else { // reallocate
            size_type newCapacity = capacityGrowth(newSize);
            const pointer newFirst = value->alloc.allocate(newCapacity);
            traits_type::move(newFirst, value->data,
                              pos); // move elements before the pos
            traits_type::move(newFirst +
                                  (count + pos), // move elements after the pos
                              value->data + pos, oldSize - pos);
            updatePointer(newFirst, newCapacity);
            traits_type::move(newFirst + pos, s,
                              count); // fill count characters
        }

        value->data[newSize] = traits_type::to_char_type(0);
        value->size = newSize;

        return *this;
    }

    cow_basic_string& insert(size_type pos, const CharT* s) {
        return insert(pos, s, traits_type::length(s));
    }

    cow_basic_string& insert(size_type pos, const cow_basic_string& str) {
        return insert(pos, str.c_str());
    }

    cow_basic_string& insert(size_type pos, const cow_basic_string& str,
                             size_type str_pos, size_type count = npos) {
        checkRange(str_pos, str.size());

        return insert(substr(str_pos, count).c_str());
    }

    iterator insert(const_iterator pos, CharT ch) {
        std::size_t offset = pos - begin();
        insert(offset, 1, ch);

        return begin() + offset;
    }

    iterator insert(const_iterator pos, size_type count, CharT ch) {
        std::size_t offset = pos - begin();
        insert(offset, count, ch);

        return begin() + offset;
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    iterator insert(const_iterator pos, InIter first, InIter last) {
        size_type offset = pos - begin();
        insert(offset, cow_basic_string(first, last));

        return begin() + offset;
    }

    iterator insert(const_iterator pos, std::initializer_list<CharT> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    // Replaces the part of the string indicated by either[pos, pos + count) or
    // [first, last)
    cow_basic_string& replace(size_type pos, size_type count,
                              const cow_basic_string& str) {
        return replace(pos, count, str.c_str(), str.size());
    }

    cow_basic_string& replace(const_iterator first, const_iterator last,
                              const cow_basic_string& str) {
        return replace(first, last, str.c_str(), str.size());
    }

    cow_basic_string& replace(size_type pos, size_type count,
                              const cow_basic_string& str, size_type pos2,
                              size_type count2 = npos) {
        return replace(pos, count, str.substr(pos2, count2));
    }

    template <typename InIter,
              typename = enable_if_t<is_iterator<InIter>::value>>
    cow_basic_string& replace(const_iterator first, const_iterator last,
                              InIter first2, InIter last2) {
        return replace(first, last, cow_basic_string(first2, last2));
    }

    cow_basic_string& replace(size_type pos, size_type count, const CharT* cstr,
                              size_type count2) {
        // replace impl
        size_type oldSize = size();
        count2 = tiny_stl::min(count2, traits_type::length(cstr));
        size_type newSize = oldSize - count + count2;

        checkLengthAndRange(newSize, pos, oldSize);

        if (this->isShared())
            value = new StringValue(value->data);

        if (newSize >= capacity() - 1) // reallocate
            reallocteHelper(newSize);

        traits_type::move(value->data + pos + count2, // dst
                          value->data + pos + count,  // src
                          oldSize - pos - count);     // count
        traits_type::move(value->data + pos, cstr, count2);
        value->size = newSize;
        value->data[newSize] = traits_type::to_char_type(0);

        return *this;
    }

    cow_basic_string& replace(const_iterator first, const_iterator last,
                              const CharT* cstr, size_type count2) {
        return replace(first - begin(), last - first, cstr, count2);
    }

    cow_basic_string& replace(size_type pos, size_type count,
                              const CharT* cstr) {
        return replace(pos, count, cstr, npos);
    }

    cow_basic_string& replace(const_iterator first, const_iterator last,
                              const CharT* cstr) {
        return replace(first, last, cstr, npos);
    }

    cow_basic_string& replace(size_type pos, size_type count, size_type count2,
                              CharT ch) {
        // replace impl
        size_type oldSize = size();
        size_type newSize = oldSize - count + count2;

        checkLengthAndRange(newSize, pos, oldSize);

        if (this->isShared())
            value = new StringValue(value->data);

        if (newSize >= capacity() - 1) // reallocate
            reallocteHelper(newSize);

        traits_type::move(value->data + pos + count2, // dst
                          value->data + pos + count,  // src
                          oldSize - pos - count);     // count
        traits_type::assign(value->data + pos, count2, ch);
        value->size = newSize;
        value->data[newSize] = traits_type::to_char_type(0);

        return *this;
    }

    cow_basic_string& replace(const_iterator first, const_iterator last,
                              size_type count2, CharT ch) {
        return replace(first - begin(), last - first, count2, ch);
    }

    cow_basic_string& replace(const_iterator first, const_iterator last,
                              std::initializer_list<CharT> ilist) {
        return replace(first - begin(), last - first, ilist.begin(),
                       ilist.end());
    }

    int compare(const cow_basic_string& rhs) const noexcept {
        size_type rlen = tiny_stl::min(size(), rhs.size());
        int cmpd = traits_type::compare(data(), rhs.data(), rlen);
        int cmps = static_cast<int>(size()) - static_cast<int>(rhs.size());

        return cmpd != 0 ? cmpd : cmps;
    }

    // compare this->[pos, pos + count) to rhs
    int compare(size_type pos1, size_type count1,
                const cow_basic_string& rhs) const {
        return substr(pos1, count1).compare(rhs);
    }

    int compare(size_type pos1, size_type count1, const cow_basic_string& rhs,
                size_type pos2, size_type count2 = npos) const {
        return substr(pos1, count1).compare(rhs.substr(pos2, count2));
    }

    int compare(const CharT* s) const {
        return compare(cow_basic_string(s));
    }

    int compare(size_type pos1, size_type count1, const CharT* s) {
        return substr(pos1, count1).compare(cow_basic_string(s));
    }

    int compare(size_type pos1, size_type count1, const CharT* s,
                size_type count2) const {
        return substr(pos1, count1).compare(cow_basic_string(s, count2));
    }

    void swap(cow_basic_string& rhs) noexcept(
        AllocTraits::propagate_on_container_swap::value ||
        AllocTraits::is_always_equal::value) {
        tiny_stl::swap(value, rhs.value);
    }

    cow_basic_string substr(size_type pos = 0, size_type count = npos) const {
        return cow_basic_string(*this, pos, count);
    }

    // copy this->[pos, pos + count) to dst
    size_type copy(CharT* dst, size_type count, size_type pos = 0) const {
        assert(pos <= size());
        size_type num = tiny_stl::min(size() - pos, count);
        traits_type::move(dst, value->data + pos, num);

        return num;
    }

public:
    // extended non-standard method

    // trim from left for the special character ch
    void ltrim(char ch) {
        if (this->isShared())
            value = new StringValue(value->data);

        this->erase(this->begin(), find_if(this->begin(), this->end(),
                                           [ch](char c) { return ch != c; }));
    }

    // trim from left for the space ' '
    void ltrim() {
        ltrim(' ');
    }

    // trim from right for the special character ch
    void rtrim(char ch) {
        if (this->isShared())
            value = new StringValue(value->data);

        this->erase(find_if(this->rbegin(), this->rend(),
                            [ch](char c) { return ch != c; })
                        .base(),
                    this->end());
    }

    // trim from right for the space ' '
    void rtrim() {
        rtrim(' ');
    }

    // trim both left and right for the special character ch
    void trim(char ch) {
        ltrim(ch);
        rtrim(ch);
    }

    // trim both left and right for the space ' '
    void trim() {
        trim(' ');
    }

private:
    // result pos -> xpos
    // if all of the following conditions are true, xpos is result
    // xpos >= pos
    // xpos + str.size() <= size()
    // Traits::eq(at(xpos + n), str.at(n))

    // find algorithm
    // pos:   this->position at which to start the search
    // count: s->length of substring to search for
    size_type findHelper(const CharT* s, size_type pos, size_type count) const {
        // Here is a naive find algorithm

        size_type thisSize = size();

        if (count > thisSize || pos > thisSize - count)
            return npos;

        if (count == 0) {
            return tiny_stl::min(pos, size() - 1);
        }

        for (size_type i = pos; i <= thisSize - count; ++i) {
            // if matched first character
            if (traits_type::eq(value->data[i], s[0])) {
                size_type j;
                for (j = 1; j < count; ++j) {
                    if (!traits_type::eq(value->data[i + j], s[j])) // mismatch
                        break;
                }
                if (j == count)
                    return i;
            }
        }

        return npos;
    }

    size_type rfindHelper(const CharT* str, size_type pos,
                          size_type count) const {
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
            if (!Traits::eq(value->data[i], str[0]))
                continue;

            size_type j = 1;
            for (; j < count; ++j) {
                if (!Traits::eq(value->data[i + j], str[j]))
                    break;
            }
            if (j == count)
                return i;
        }
        return npos;
    }

public:
    // operator basic_string_view<CharT, Traits>() const noexcept
    //{
    //    return basic_string_view<CharT, Traits>{data(), size()};
    //}

public:
    size_type find(const cow_basic_string& str,
                   size_type pos = 0) const noexcept {
        return findHelper(str.c_str(), pos, str.size());
    }

    size_type find(const CharT* s, size_type pos, size_type count) const {
        return findHelper(s, pos, count);
    }

    size_type find(const CharT* s, size_type pos = 0) const {
        return findHelper(s, pos, traits_type::length(s));
    }

    size_type find(CharT ch, size_type pos = 0) const {
        const CharT* find_at =
            traits_type::find(value->data + pos, size() - pos, ch);
        return find_at == nullptr
                   ? npos
                   : static_cast<size_type>(find_at - value->data);
    }

    size_type rfind(const cow_basic_string& str,
                    size_type pos = npos) const noexcept {
        return rfindHelper(str.c_str(), pos, str.size());
    }

    size_type rfind(const CharT* s, size_type pos, size_type count) const {
        return rfindHelper(s, pos, count);
    }

    size_type rfind(const CharT* s, size_type pos = npos) const {
        return rfindHelper(s, pos, traits_type::length(s));
    }

    size_type rfind(CharT ch, size_type pos = npos) const {
        pos = tiny_stl::min(pos, size() - 1);
        CharT str[2];
        str[0] = ch;
        str[1] = traits_type::to_char_type(0);
        return rfindHelper(str, pos, 1);
    }

    // find_first_of, find_first_not_of, find_last_of, find_last_not_of
    // no implement

    // Provide non-standard interface to improve efficiency of reading
    const_reference c_front() const {
        return front();
    }

    const_reference c_back() const {
        return back();
    }

    const_pointer c_data() const noexcept {
        return data();
    }

    const_reference c_at(size_type pos) const {
        return at(pos);
    }

private:
    bool isShared() const noexcept {
        return value->isShared();
    }

    std::size_t getRefCount() const noexcept {
        return value->getRefCount();
    }

    [[noreturn]] static void xLength() {
        throw "string_base<CharT> too long";
    }

    [[noreturn]] static void xRange() {
        throw "invalid string_base<CharT> subscript";
    }

}; // class basic_string<CharT, Traits, Alloc>

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(const cow_basic_string<CharT, Traits, Alloc>& lhs,
          const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    cow_basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(lhs.size() + rhs.size());
    tmp += lhs;
    tmp += rhs;

    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(const CharT* lhs, const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    cow_basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(Traits::length(lhs), rhs.size());
    tmp += lhs;
    tmp += rhs;

    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(CharT lhs, const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    cow_basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(1 + rhs.size());
    tmp += lhs;
    tmp += rhs;

    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(const cow_basic_string<CharT, Traits, Alloc>& lhs, const CharT* rhs) {
    cow_basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(lhs.size() + Traits::length(rhs));
    tmp += lhs;
    tmp += rhs;

    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(const cow_basic_string<CharT, Traits, Alloc>& lhs, CharT rhs) {
    cow_basic_string<CharT, Traits, Alloc> tmp;
    tmp.reserve(lhs.size() + 1);
    tmp += lhs;
    tmp += rhs;

    return tmp;
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(cow_basic_string<CharT, Traits, Alloc>&& lhs,
          const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    return tiny_stl::move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(const cow_basic_string<CharT, Traits, Alloc>& lhs,
          cow_basic_string<CharT, Traits, Alloc>&& rhs) {
    return tiny_stl::move(rhs.insert(0, lhs));
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(cow_basic_string<CharT, Traits, Alloc>&& lhs,
          cow_basic_string<CharT, Traits, Alloc>&& rhs) {
    return tiny_stl::move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(const CharT* lhs, cow_basic_string<CharT, Traits, Alloc>&& rhs) {
    return tiny_stl::move(rhs.insert(0, lhs));
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(CharT lhs, cow_basic_string<CharT, Traits, Alloc>&& rhs) {
    return tiny_stl::move(rhs.insert(0, 1, lhs));
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(cow_basic_string<CharT, Traits, Alloc>&& lhs, const CharT* rhs) {
    return tiny_stl::move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
cow_basic_string<CharT, Traits, Alloc>
operator+(cow_basic_string<CharT, Traits, Alloc>&& lhs, CharT rhs) {
    lhs.push_back(rhs);
    return tiny_stl::move(lhs);
}

template <typename CharT, typename Traits, typename Alloc>
inline bool
operator==(const cow_basic_string<CharT, Traits, Alloc>& lhs,
           const cow_basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return lhs.size() == rhs.size() &&
           tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename CharT, typename Traits, typename Alloc>
inline bool
operator!=(const cow_basic_string<CharT, Traits, Alloc>& lhs,
           const cow_basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename CharT, typename Traits, typename Alloc>
inline bool
operator<(const cow_basic_string<CharT, Traits, Alloc>& lhs,
          const cow_basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename CharT, typename Traits, typename Alloc>
inline bool
operator>(const cow_basic_string<CharT, Traits, Alloc>& lhs,
          const cow_basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return rhs < lhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool
operator<=(const cow_basic_string<CharT, Traits, Alloc>& lhs,
           const cow_basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(rhs < lhs);
}

template <typename CharT, typename Traits, typename Alloc>
inline bool
operator>=(const cow_basic_string<CharT, Traits, Alloc>& lhs,
           const cow_basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs < rhs);
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator==(const CharT* clhs,
                       const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    const cow_basic_string<CharT, Traits, Alloc> lhs(clhs);
    return lhs == rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator==(const cow_basic_string<CharT, Traits, Alloc>& rhs,
                       const CharT* crhs) {
    const cow_basic_string<CharT, Traits, Alloc> lhs(crhs);
    return lhs == rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator!=(const CharT* clhs,
                       const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    const cow_basic_string<CharT, Traits, Alloc> lhs(clhs);
    return lhs != rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator!=(const cow_basic_string<CharT, Traits, Alloc>& lhs,
                       const CharT* crhs) {
    const cow_basic_string<CharT, Traits, Alloc> rhs(crhs);
    return lhs != rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<(const CharT* clhs,
                      const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    const cow_basic_string<CharT, Traits, Alloc> lhs(clhs);
    return lhs < rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<(const cow_basic_string<CharT, Traits, Alloc>& lhs,
                      const CharT* crhs) {
    const cow_basic_string<CharT, Traits, Alloc> rhs(crhs);
    return lhs < rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>(const CharT* clhs,
                      const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    const cow_basic_string<CharT, Traits, Alloc> lhs(clhs);
    return lhs > rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>(const cow_basic_string<CharT, Traits, Alloc>& lhs,
                      const CharT* crhs) {
    const cow_basic_string<CharT, Traits, Alloc> rhs(crhs);
    return lhs > rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<=(const CharT* clhs,
                       const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    const cow_basic_string<CharT, Traits, Alloc> lhs(clhs);
    return lhs <= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator<=(const cow_basic_string<CharT, Traits, Alloc>& lhs,
                       const CharT* crhs) {
    const cow_basic_string<CharT, Traits, Alloc> rhs(crhs);
    return lhs <= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>=(const CharT* clhs,
                       const cow_basic_string<CharT, Traits, Alloc>& rhs) {
    const cow_basic_string<CharT, Traits, Alloc> lhs(clhs);
    return lhs >= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
inline bool operator>=(const cow_basic_string<CharT, Traits, Alloc>& lhs,
                       const CharT* crhs) {
    const cow_basic_string<CharT, Traits, Alloc> rhs(crhs);
    return lhs >= rhs;
}

template <typename CharT, typename Traits, typename Alloc>
void swap(cow_basic_string<CharT, Traits, Alloc>& lhs,
          cow_basic_string<CharT, Traits, Alloc>&
              rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename CharT, typename Traits, typename Alloc>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const cow_basic_string<CharT, Traits, Alloc>& str) {
    // no format output
    os << str.c_str();
    return os;
}

template <typename CharT, typename Traits, typename Alloc>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is,
           cow_basic_string<CharT, Traits, Alloc>& str) {
    if (str.isShared())
        str = cow_basic_string<CharT, Traits, Alloc>(str.data());

    // no format input
    is >> str.data();

    return is;
}

template <typename CharT, typename Traits, typename Alloc>
struct hash<cow_basic_string<CharT, Traits, Alloc>> {
    using argument_type = cow_basic_string<CharT, Traits, Alloc>;
    using result_type = std::size_t;

    std::size_t
    operator()(const cow_basic_string<CharT, Traits, Alloc>& str) const
        noexcept {
        return tiny_stl::hashFNV(str.c_str(), str.size());
    }
};

using cow_string = cow_basic_string<char>;
using cow_wstring = cow_basic_string<wchar_t>;
using cow_u16string = cow_basic_string<char16_t>;
using cow_u32string = cow_basic_string<char32_t>;

namespace {

template <typename CharT, typename T>
inline cow_basic_string<CharT> IntegerToCowStringAux(T val, const CharT* zero) {
    bool isNegative = val < 0;
    CharT buffer[21]; // -2^63 ~ 2^64-1
    CharT* ptr = buffer;
    do {
        int lp = static_cast<int>(val % 10);
        val /= 10;
        *ptr++ = zero[lp];
    } while (val);

    if (isNegative) {
        *ptr++ = cow_basic_string<CharT>::traits_type::to_char_type('-');
    }
    *ptr = cow_basic_string<CharT>::traits_type::to_char_type('\0');
    tiny_stl::reverse(buffer, ptr);
    return cow_basic_string<CharT>{buffer};
}

template <typename CharT, typename T>
inline cow_basic_string<CharT> IntegerToCowStringHelp(T val, true_type) {
    static const char digits[] = "9876543210123456789";
    static const char* zero = digits + 9;
    return IntegerToCowStringAux(val, zero);
}

template <typename CharT, typename T>
inline cow_basic_string<CharT> IntegerToCowStringHelp(T val, false_type) {
    static const wchar_t digits[] = L"9876543210123456789";
    static const wchar_t* zero = digits + 9;
    return IntegerToCowStringAux(val, zero);
}

template <
    typename CharT, typename T,
    enable_if_t<is_same_v<CharT, char> || is_same_v<CharT, wchar_t>, int> = 0>
inline cow_basic_string<CharT> IntegerToCowString(T val) {
    static_assert(is_integral_v<T>, "T must be integral");
    static_assert(is_same_v<CharT, char> || is_same_v<CharT, wchar_t>,
                  "CharT must be char or wchar_t");
    return IntegerToCowStringHelp<CharT>(val, is_same<CharT, char>());
}

} // namespace

inline cow_string to_cow_string(int value) {
    return IntegerToCowString<char>(value);
}

inline cow_string to_cow_string(long value) {
    return IntegerToCowString<char>(value);
}

inline cow_string to_cow_string(unsigned value) {
    return IntegerToCowString<char>(value);
}

inline cow_string to_cow_string(unsigned long value) {
    return IntegerToCowString<char>(value);
}

inline cow_string to_cow_string(long long value) {
    return IntegerToCowString<char>(value);
}

inline cow_string to_cow_string(unsigned long long value) {
    return IntegerToCowString<char>(value);
}

inline cow_wstring to_cow_wstring(int value) {
    return IntegerToCowString<wchar_t>(value);
}

inline cow_wstring to_cow_wstring(long value) {
    return IntegerToCowString<wchar_t>(value);
}

inline cow_wstring to_cow_wstring(unsigned value) {
    return IntegerToCowString<wchar_t>(value);
}

inline cow_wstring to_cow_wstring(unsigned long value) {
    return IntegerToCowString<wchar_t>(value);
}

inline cow_wstring to_cow_wstring(long long value) {
    return IntegerToCowString<wchar_t>(value);
}

inline cow_wstring to_cow_wstring(unsigned long long value) {
    return IntegerToCowString<wchar_t>(value);
}

// const cow_string is effective
// non-const cow_string as far as possible use operations of const version
// e.g.
// cow_string s;
// s.begin() -> s.cbegin()
//
// moreover, to distinguish between reading and writing
// many additional and non-standard operations of const version is provided
// such as, cfront(), cback(), cdata()
// to avoid copying of data

} // namespace tiny_stl
