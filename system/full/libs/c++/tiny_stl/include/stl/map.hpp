// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "rbtree.hpp"

namespace tiny_stl {

template <typename Key, typename T, typename Compare = less<Key>,
          typename Alloc = allocator<pair<Key, T>>>
class map : public RBTree<pair<Key, T>, Compare, Alloc, true> {
public:
    using allocator_type = Alloc;

private:
    using Base = RBTree<pair<Key, T>, Compare, Alloc, true>;
    using AlTraits = allocator_traits<allocator_type>;
    using AlNode = typename Base::AlNode;
    using AlNodeTraits = typename Base::AlNodeTraits;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = pair<const Key, T>;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename AlTraits::pointer;
    using const_pointer = typename AlTraits::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = typename Base::reverse_iterator;
    using const_reverse_iterator = typename Base::const_reverse_iterator;

public:
    class value_compare {
        friend map;

    protected:
        Compare mCmp;

        value_compare(Compare c) : mCmp(c) {
        }

    public:
        bool operator()(const value_type& lhs, const value_type& rhs) const {
            return mCmp(lhs.first, rhs.first);
        }
    };

public:
    map() : map(Compare()) {
    }
    explicit map(const Compare& cmp, const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
    }

    explicit map(const Alloc& alloc) : Base(Compare(), alloc) {
    }

    template <typename InIter>
    map(InIter first, InIter last, const Compare& cmp = Compare(),
        const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_unique(first, last);
    }

    template <typename InIter>
    map(InIter first, InIter last, const Alloc& alloc)
        : Base(Compare(), alloc) {
        this->insert_unique(first, last);
    }

    map(const map& rhs)
        : Base(rhs, AlTraits::select_on_container_copy_construction(
                        rhs.get_allocator())) {
    }

    map(const map& rhs, const Alloc& alloc) : Base(rhs, alloc) {
    }

    map(map&& rhs) noexcept : Base(tiny_stl::move(rhs)) {
    }

    map(map&& rhs, const Alloc& alloc) : Base(tiny_stl::move(rhs), alloc) {
    }

    map(std::initializer_list<value_type> ilist, const Compare& cmp = Compare(),
        const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_unique(ilist.begin(), ilist.end());
    }

    map(std::initializer_list<value_type> ilist, const Alloc& alloc)
        : map(ilist, Compare(), alloc) {
    }

    map& operator=(const map& rhs) {
        Base::operator=(rhs);
        return *this;
    }

    map& operator=(map&& rhs) {
        Base::operator=(tiny_stl::move(rhs));
        return *this;
    }

    map& operator=(std::initializer_list<value_type> ilist) {
        map tmp(ilist);
        this->swap(tmp);
        return *this;
    }

    T& at(const Key& key) {
        iterator pos = this->find(key);
        if (pos == this->end())
            xRange();

        return pos->second;
    }

    const T& at(const Key& key) const {
        const_iterator pos = this->find(key);
        if (pos == this->end())
            xRange();

        return pos->second;
    }

    T& operator[](const Key& key) {
        iterator pos = this->find(key);
        if (pos == this->end())
            return this->insert(make_pair(key, T{})).first->second;

        return pos->second;
    }

    T& operator[](Key&& key) {
        iterator pos = this->find(key);
        if (pos == this->end())
            return this->insert(make_pair(tiny_stl::move(key), T{}))
                .first->second;

        return pos->second;
    }

    pair<iterator, bool> insert(const value_type& val) {
        return this->insert_unique(val);
    }

    template <typename P,
              typename = enable_if_t<is_constructible<value_type, P&&>::value>>
    pair<iterator, bool> insert(P&& val) {
        return this->insert_unique(tiny_stl::forward<P>(val));
    }

    pair<iterator, bool> insert(value_type&& val) {
        return this->insert_unique(tiny_stl::move(val));
    }

    template <typename InIter>
    void insert(InIter first, InIter last) {
        this->insert_unique(first, last);
    }

    void insert(std::initializer_list<value_type> ilist) {
        this->insert_unique(ilist.begin(), ilist.end());
    }

    template <typename... Args>
    pair<iterator, bool> emplace(Args&&... args) {
        return this->emplace_unique(tiny_stl::forward<Args>(args)...);
    }

    void swap(map& rhs) {
        Base::swap(rhs);
    }

    key_compare key_comp() const {
        return key_compare{};
    }

    value_compare value_comp() const {
        return value_compare{key_comp()};
    }

private:
    [[noreturn]] static void xRange() {
        throw "map<Key, T>, key is not exist";
    }
}; // map

template <typename Key, typename T, typename Cmp, typename Alloc>
inline void
swap(map<Key, T, Cmp, Alloc>& lhs,
     map<Key, T, Cmp, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

template <typename Key, typename T, typename Compare = less<Key>,
          typename Alloc = allocator<pair<Key, T>>>
class multimap : public RBTree<pair<Key, T>, Compare, Alloc, true> {
public:
    using allocator_type = Alloc;

private:
    using Base = RBTree<pair<Key, T>, Compare, Alloc, true>;
    using AlNode = typename Base::AlNode;
    using AlNodeTraits = typename Base::AlNodeTraits;
    using AlTraits = allocator_traits<allocator_type>;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = pair<const Key, T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename AlTraits::pointer;
    using const_pointer = typename AlTraits::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = typename Base::reverse_iterator;
    using const_reverse_iterator = typename Base::const_reverse_iterator;

public:
    class value_compare {
        friend multimap;

    protected:
        Compare mCmp;

        value_compare(Compare c) : mCmp(c) {
        }

    public:
        bool operator()(const value_type& lhs, const value_type& rhs) const {
            return mCmp(lhs.first, rhs.first);
        }
    };

public:
    multimap() : multimap(Compare()) {
    }
    explicit multimap(const Compare& cmp, const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
    }

    explicit multimap(const Alloc& alloc) : Base(Compare(), alloc) {
    }

    template <typename InIter>
    multimap(InIter first, InIter last, const Compare& cmp = Compare(),
             const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_equal(first, last);
    }

    template <typename InIter>
    multimap(InIter first, InIter last, const Alloc& alloc)
        : Base(Compare(), alloc) {
        this->insert_equal(first, last);
    }

    multimap(const multimap& rhs)
        : Base(rhs, AlTraits::select_on_container_copy_construction(
                        rhs.get_allocator())) {
    }

    multimap(const multimap& rhs, const Alloc& alloc) : Base(rhs, alloc) {
    }

    multimap(multimap&& rhs) : Base(tiny_stl::move(rhs)) {
    }

    multimap(multimap&& rhs, const Alloc& alloc)
        : Base(tiny_stl::move(rhs), alloc) {
    }

    multimap(std::initializer_list<value_type> ilist,
             const Compare& cmp = Compare(), const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_equal(ilist.begin(), ilist.end());
    }

    multimap(std::initializer_list<value_type> ilist, const Alloc& alloc)
        : multimap(ilist, Compare(), alloc) {
    }

    multimap& operator=(const multimap& rhs) {
        Base::operator=(rhs);
        return *this;
    }

    multimap& operator=(multimap&& rhs) {
        Base::operator=(tiny_stl::move(rhs));
        return *this;
    }

    multimap& operator=(std::initializer_list<value_type> ilist) {
        multimap tmp(ilist);
        this->swap(tmp);
        return *this;
    }

    pair<iterator, bool> insert(const value_type& val) {
        return this->insert_equal(val);
    }

    template <typename P,
              typename = enable_if_t<is_constructible<value_type, P&&>::value>>
    pair<iterator, bool> insert(P&& val) {
        return this->insert_equal(tiny_stl::forward<P>(val));
    }

    pair<iterator, bool> insert(value_type&& val) {
        return this->insert_equal(tiny_stl::move(val));
    }

    template <typename InIter>
    void insert(InIter first, InIter last) {
        this->insert_equal(first, last);
    }

    void insert(std::initializer_list<value_type> ilist) {
        this->insert_equal(ilist.begin(), ilist.end());
    }

    template <typename... Args>
    pair<iterator, bool> emplace(Args&&... args) {
        return this->emplace_equal(tiny_stl::forward<Args>(args)...);
    }

    void swap(multimap& rhs) {
        Base::swap(rhs);
    }

    key_compare key_comp() const {
        return key_compare{};
    }

    value_compare value_comp() const {
        return value_compare{key_comp()};
    }

private:
    [[noreturn]] static void xRange() {
        throw "multimap<Key, T>, key is not exist";
    }
}; // multimap

template <typename Key, typename T, typename Cmp, typename Alloc>
inline void
swap(multimap<Key, T, Cmp, Alloc>& lhs,
     multimap<Key, T, Cmp, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace tiny_stl
