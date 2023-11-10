// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "rbtree.hpp"

namespace tiny_stl {

// set
template <typename Key, typename Compare = tiny_stl::less<Key>,
          typename Alloc = tiny_stl::allocator<Key>>
class set : public RBTree<Key, Compare, Alloc, false> {
public:
    using allocator_type = Alloc;
private:
    using Base = RBTree<Key, Compare, allocator_type, false>;
    using AlTraits = allocator_traits<allocator_type>;
    using AlNode = typename Base::AlNode;
    using AlNodeTraits = typename Base::AlNodeTraits;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using key_compare = Compare;
    using value_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename AlTraits::pointer;
    using const_pointer = typename AlTraits::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = typename Base::reverse_iterator;
    using const_reverse_iterator = typename Base::const_reverse_iterator;

public:
    set() : set(Compare()) {
    }
    explicit set(const Compare& cmp, const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
    }

    explicit set(const Alloc& alloc) : Base(key_compare(), alloc) {
    }

    template <typename InIter>
    set(InIter first, InIter last, const key_compare& cmp,
        const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_unique(first, last);
    }

    template <typename InIter>
    set(InIter first, InIter last, const Alloc& alloc)
        : Base(key_compare(), alloc) {
        this->insert_unique(first, last);
    }

    set(const set& rhs)
        : Base(rhs, AlTraits::select_on_container_copy_construction(
                        rhs.get_allocator())) {
    }

    set(const set& rhs, const Alloc& alloc) : Base(rhs, alloc) {
    }

    set(set&& rhs) : Base(tiny_stl::move(rhs)) {
    }

    set(set&& rhs, const Alloc& alloc) : Base(tiny_stl::move(rhs), alloc) {
    }

    set(std::initializer_list<value_type> ilist, const Compare& cmp = Compare(),
        const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_unique(ilist.begin(), ilist.end());
    }

    set(std::initializer_list<value_type> ilist, const Alloc& alloc)
        : set(ilist, Compare(), alloc) {
    }

    set& operator=(const set& rhs) {
        Base::operator=(rhs);
        return *this;
    }

    set& operator=(set&& rhs) {
        Base::operator=(tiny_stl::move(rhs));
        return *this;
    }

    set& operator=(std::initializer_list<value_type> ilist) {
        set tmp(ilist);
        this->swap(tmp);
        return *this;
    }

    pair<iterator, bool> insert(const value_type& val) {
        return this->insert_unique(val);
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

    void swap(set& rhs) {
        Base::swap(rhs);
    }

    key_compare key_comp() const {
        return key_compare{};
    }

    value_compare value_comp() const {
        return value_compare{};
    }
}; // set

template <typename Key, typename Compare, typename Alloc>
inline void
swap(set<Key, Compare, Alloc>& lhs,
     set<Key, Compare, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

// multiset
template <typename Key, typename Compare = tiny_stl::less<Key>,
          typename Alloc = tiny_stl::allocator<Key>>
class multiset : public RBTree<Key, Compare, Alloc, false> {
public:
    using allocator_type = Alloc;
private:
    using Base = RBTree<Key, Compare, allocator_type, false>;
    using AlTraits = allocator_traits<allocator_type>;
    using AlNode = typename Base::AlNode;
    using AlNodeTraits = typename Base::AlNodeTraits;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using value_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename AlTraits::pointer;
    using const_pointer = typename AlTraits::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = typename Base::reverse_iterator;
    using const_reverse_iterator = typename Base::const_reverse_iterator;

public:
    multiset() : multiset(Compare()) {
    }
    explicit multiset(const Compare& cmp, const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
    }

    explicit multiset(const Alloc& alloc) : Base(key_compare(), alloc) {
    }

    template <typename InIter>
    multiset(InIter first, InIter last, const key_compare& cmp,
             const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_equal(first, last);
    }

    template <typename InIter>
    multiset(InIter first, InIter last, const Alloc& alloc)
        : Base(key_compare(), alloc) {
        this->insert_equal(first, last);
    }

    multiset(const multiset& rhs)
        : Base(rhs, AlTraits::select_on_container_copy_construction(
                        rhs.get_allocator())) {
    }

    multiset(const multiset& rhs, const Alloc& alloc) : Base(rhs, alloc) {
    }

    multiset(multiset&& rhs) : Base(tiny_stl::move(rhs)) {
    }

    multiset(multiset&& rhs, const Alloc& alloc)
        : Base(tiny_stl::move(rhs), alloc) {
    }

    multiset(std::initializer_list<value_type> ilist,
             const Compare& cmp = Compare(), const Alloc& alloc = Alloc())
        : Base(cmp, alloc) {
        this->insert_equal(ilist.begin(), ilist.end());
    }

    multiset(std::initializer_list<value_type> ilist, const Alloc& alloc)
        : multiset(ilist, Compare(), alloc) {
    }

    multiset& operator=(const multiset& rhs) {
        Base::operator=(rhs);
        return *this;
    }

    multiset& operator=(multiset&& rhs) {
        Base::operator=(tiny_stl::move(rhs));
        return *this;
    }

    multiset& operator=(std::initializer_list<value_type> ilist) {
        multiset tmp(ilist);
        this->swap(tmp);
        return *this;
    }

    iterator insert(const value_type& val) {
        return this->insert_equal(val);
    }

    iterator insert(value_type&& val) {
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
    iterator emplace(Args&&... args) {
        return this->insert_equal(tiny_stl::forward<Args>(args)...);
    }

    void swap(multiset& rhs) {
        Base::swap(rhs);
    }

    key_compare key_comp() const {
        return key_compare{};
    }

    value_compare value_comp() const {
        return value_compare{};
    }
}; // multiset

template <typename Key, typename Compare, typename Alloc>
inline void
swap(multiset<Key, Compare, Alloc>& lhs,
     multiset<Key, Compare, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace tiny_stl