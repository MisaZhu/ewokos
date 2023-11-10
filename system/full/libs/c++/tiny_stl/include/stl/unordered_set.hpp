// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "hashtable.hpp"

namespace tiny_stl {

template <typename Key, typename Hash = hash<Key>,
          typename KeyEqual = equal_to<Key>, typename Alloc = allocator<Key>>
class unordered_set : public HashTable<Key, Hash, KeyEqual, Alloc, false> {
public:
    using allocator_type = Alloc;
private:
    using Base = HashTable<Key, Hash, KeyEqual, Alloc, false>;
    using AlTraits = allocator_traits<allocator_type>;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename AlTraits::pointer;
    using const_pointer = typename AlTraits::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using local_iterator = typename Base::local_iterator;
    using const_local_iterator = typename Base::const_local_iterator;

public:
    // (1)
    unordered_set() : unordered_set(0) {
    }
    explicit unordered_set(size_type num_bucket, const Hash& hashfunc = Hash(),
                           const KeyEqual& eq = KeyEqual(),
                           const Alloc& alloc = Alloc())
        : Base(num_bucket, alloc, hashfunc, eq) {
    }

    // (1)
    unordered_set(size_type num_bucket, const Alloc& alloc)
        : unordered_set(num_bucket, Hash(), KeyEqual(), alloc) {
    }

    // (1)
    unordered_set(size_type num_bucket, const Hash& hashfunc,
                  const Alloc& alloc)
        : unordered_set(num_bucket, hashfunc, KeyEqual(), alloc) {
    }

    // (1)
    explicit unordered_set(const Alloc& alloc) : Base(0, alloc) {
    }

    // (2)
    template <typename InIter>
    unordered_set(InIter first, InIter last, size_type num_bucket = 0,
                  const Hash& hashfunc = Hash(),
                  const KeyEqual& eq = KeyEqual(), const Alloc& alloc = Alloc())
        : Base(num_bucket, alloc, hashfunc, eq) {
        this->insert_unique(first, last);
    }

    // (2)
    template <typename InIter>
    unordered_set(InIter first, InIter last, size_type num_bucket,
                  const Alloc& alloc)
        : unordered_set(first, last, num_bucket, Hash(), KeyEqual(), alloc) {
    }

    // (2)
    template <typename InIter>
    unordered_set(InIter first, InIter last, size_type num_bucket,
                  const Hash& hashfunc, const Alloc& alloc)
        : unordered_set(first, last, num_bucket, hashfunc, KeyEqual(), alloc) {
    }

    // (3)
    unordered_set(const unordered_set& rhs) : Base(rhs) {
    }

    // (3)
    unordered_set(const unordered_set& rhs, const Alloc& alloc)
        : Base(rhs, alloc) {
    }

    // (4)
    unordered_set(unordered_set&& rhs) noexcept : Base(tiny_stl::move(rhs)) {
    }

    // (5)
    unordered_set(std::initializer_list<value_type> ilist,
                  size_type num_bucket = 0, const Hash& hashfunc = Hash(),
                  const KeyEqual& eq = KeyEqual(), const Alloc& alloc = Alloc())
        : unordered_set(ilist.begin(), ilist.end(), num_bucket, hashfunc, eq,
                        alloc) {
    }

    // (5)
    unordered_set(std::initializer_list<value_type> ilist, size_type num_bucket,
                  const Alloc& alloc)
        : unordered_set(ilist, num_bucket, Hash(), KeyEqual(), alloc) {
    }

    // (5)
    unordered_set(std::initializer_list<value_type> ilist, size_type num_bucket,
                  const Hash& hashfunc, const Alloc& alloc)
        : unordered_set(ilist, num_bucket, hashfunc, KeyEqual(), alloc) {
    }

    unordered_set& operator=(const unordered_set& rhs) {
        Base::operator=(rhs);
        return *this;
    }

    unordered_set& operator=(unordered_set&& rhs) {
        Base::operator=(tiny_stl::move(rhs));
        return *this;
    }

    unordered_set& operator=(std::initializer_list<value_type> ilist) {
        this->clear();
        this->insert(ilist);
        return *this;
    }

    size_type count(const key_type& key) const {
        return this->count_unique(key);
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

    void swap(unordered_set& rhs) {
        Base::swap(rhs);
    }
}; // unordered_set

template <typename Key, typename Hash, typename KeyEqual, typename Alloc>
void swap(unordered_set<Key, Hash, KeyEqual, Alloc>& lhs,
          unordered_set<Key, Hash, KeyEqual, Alloc>& rhs) {
    lhs.swap(rhs);
}

template <typename Key, typename Hash = hash<Key>,
          typename KeyEqual = equal_to<Key>, typename Alloc = allocator<Key>>
class unordered_multiset : public HashTable<Key, Hash, KeyEqual, Alloc, false> {
public:
    using allocator_type = Alloc;
private:
    using Base = HashTable<Key, Hash, KeyEqual, Alloc, false>;
    using AlTraits = allocator_traits<allocator_type>;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename AlTraits::pointer;
    using const_pointer = typename AlTraits::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using local_iterator = typename Base::local_iterator;
    using const_local_iterator = typename Base::const_local_iterator;

public:
    // (1)
    unordered_multiset() : unordered_multiset(0) {
    }
    explicit unordered_multiset(size_type num_bucket,
                                const Hash& hashfunc = Hash(),
                                const KeyEqual& eq = KeyEqual(),
                                const Alloc& alloc = Alloc())
        : Base(num_bucket, alloc, hashfunc, eq) {
    }

    // (1)
    unordered_multiset(size_type num_bucket, const Alloc& alloc)
        : unordered_multiset(num_bucket, Hash(), KeyEqual(), alloc) {
    }

    // (1)
    unordered_multiset(size_type num_bucket, const Hash& hashfunc,
                       const Alloc& alloc)
        : unordered_multiset(num_bucket, hashfunc, KeyEqual(), alloc) {
    }

    // (1)
    explicit unordered_multiset(const Alloc& alloc) : Base(0, alloc) {
    }

    // (2)
    template <typename InIter>
    unordered_multiset(InIter first, InIter last, size_type num_bucket = 0,
                       const Hash& hashfunc = Hash(),
                       const KeyEqual& eq = KeyEqual(),
                       const Alloc& alloc = Alloc())
        : Base(num_bucket, alloc, hashfunc, eq) {
        this->insert_equal(first, last);
    }

    // (2)
    template <typename InIter>
    unordered_multiset(InIter first, InIter last, size_type num_bucket,
                       const Alloc& alloc)
        : unordered_multiset(first, last, num_bucket, Hash(), KeyEqual(),
                             alloc) {
    }

    // (2)
    template <typename InIter>
    unordered_multiset(InIter first, InIter last, size_type num_bucket,
                       const Hash& hashfunc, const Alloc& alloc)
        : unordered_multiset(first, last, num_bucket, hashfunc, KeyEqual(),
                             alloc) {
    }

    // (3)
    unordered_multiset(const unordered_multiset& rhs) : Base(rhs) {
    }

    // (3)
    unordered_multiset(const unordered_multiset& rhs, const Alloc& alloc)
        : Base(rhs, alloc) {
    }

    // (4)
    unordered_multiset(unordered_multiset&& rhs) : Base(tiny_stl::move(rhs)) {
    }

    // (5)
    unordered_multiset(std::initializer_list<value_type> ilist,
                       size_type num_bucket = 0, const Hash& hashfunc = Hash(),
                       const KeyEqual& eq = KeyEqual(),
                       const Alloc& alloc = Alloc())
        : unordered_multiset(ilist.begin(), ilist.end(), num_bucket, hashfunc,
                             eq, alloc) {
    }

    // (5)
    unordered_multiset(std::initializer_list<value_type> ilist,
                       size_type num_bucket, const Alloc& alloc)
        : unordered_multiset(ilist, num_bucket, Hash(), KeyEqual(), alloc) {
    }

    // (5)
    unordered_multiset(std::initializer_list<value_type> ilist,
                       size_type num_bucket, const Hash& hashfunc,
                       const Alloc& alloc)
        : unordered_multiset(ilist, num_bucket, hashfunc, KeyEqual(), alloc) {
    }

    unordered_multiset& operator=(const unordered_multiset& rhs) {
        Base::operator=(rhs);
        return *this;
    }

    unordered_multiset& operator=(unordered_multiset&& rhs) {
        Base::operator=(tiny_stl::move(rhs));
        return *this;
    }

    unordered_multiset& operator=(std::initializer_list<value_type> ilist) {
        this->clear();
        this->insert(ilist);
        return *this;
    }

    size_type count(const key_type& key) const {
        return this->count_equal(key);
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
        return this->emplace_equal(tiny_stl::forward<Args>(args)...);
    }

    void swap(unordered_multiset& rhs) {
        Base::swap(rhs);
    }
}; // unordered_multiset

template <typename Key, typename Hash, typename KeyEqual, typename Alloc>
void swap(unordered_multiset<Key, Hash, KeyEqual, Alloc>& lhs,
          unordered_multiset<Key, Hash, KeyEqual, Alloc>& rhs) {
    lhs.swap(rhs);
}

} // namespace tiny_stl