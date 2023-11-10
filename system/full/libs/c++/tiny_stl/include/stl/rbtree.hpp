// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <initializer_list>

#include "memory.hpp"

namespace tiny_stl {

namespace {

enum class Color : uint16_t { RED, BLACK };

template <typename T>
struct RBTNode {
    using IsNil = uint16_t;
    Color color;
    IsNil isNil; // 1 is nil, 0 is not nil
    RBTNode* parent;
    RBTNode* left;
    RBTNode* right;
    T value;
};

template <typename T>
inline RBTNode<T>* rbTreeMinValue(RBTNode<T>* ptr) {
    while (!ptr->left->isNil)
        ptr = ptr->left;

    return ptr;
}

template <typename T>
inline RBTNode<T>* rbTreeMaxValue(RBTNode<T>* ptr) {
    while (!ptr->right->isNil)
        ptr = ptr->right;

    return ptr;
}

//
//        |                                   |
//        x                                   y
//      ↙  ↘          left-rotate         ↙ ↘
//     a     y      ---------------->     x     c
//         ↙  ↘                       ↙ ↘
//         b    c                      a    b
//
template <typename T>
inline void rbTreeLeftRotate(RBTNode<T>*& root, RBTNode<T>* x) {
    RBTNode<T>* y = x->right;

    x->right = y->left;
    if (!y->left->isNil)
        y->left->parent = x;

    y->parent = x->parent;

    if (x->parent->isNil)
        root = y;
    else if (x == x->parent->left) // x is the left child of x.p
        x->parent->left = y;
    else
        x->parent->right = y;

    x->parent = y;
    y->left = x;
}

//        |                                 |
//        y                                 x
//      ↙ ↘         right-rotate        ↙ ↘
//     x     c      ---------------->    a    y
//   ↙ ↘                                   ↙ ↘
//  a    b                                  b    c
//
template <typename T>
inline void rbTreeRightRotate(RBTNode<T>*& root, RBTNode<T>* y) {
    RBTNode<T>* x = y->left;

    y->left = x->right;
    if (!x->right->isNil)
        x->right->parent = y;

    x->parent = y->parent;

    if (y->parent->isNil)
        root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;

    x->right = y;
    y->parent = x;
}

} // namespace

template <typename T>
struct RBTreeConstIterator {
    using iterator_category = bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    using Ptr = RBTNode<T>*;

    Ptr ptr = nullptr;

    RBTreeConstIterator() = default;
    RBTreeConstIterator(Ptr x) : ptr(x) {
    }
    RBTreeConstIterator(const RBTreeConstIterator& rhs) = default;
    RBTreeConstIterator& operator=(const RBTreeConstIterator& rhs) = default;

    reference operator*() const {
        return ptr->value;
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    RBTreeConstIterator& operator++() {
        if (!ptr->right->isNil) {
            ptr = ptr->right;
            ptr = rbTreeMinValue(ptr);
        } else {
            Ptr x = ptr->parent;
            while (!x->isNil && ptr == x->right) {
                ptr = x;
                x = x->parent;
            }
            ptr = x;
        }

        return *this;
    }

    RBTreeConstIterator operator++(int) {
        RBTreeConstIterator tmp = *this;
        ++*this;

        return tmp;
    }

    RBTreeConstIterator& operator--() {
        if (ptr->isNil)
            ptr = ptr->right;
        else if (!ptr->left->isNil) {
            ptr = ptr->left;
            ptr = rbTreeMaxValue(ptr);
        } else {
            Ptr x = ptr->parent;
            while (!x->isNil && ptr == x->left) {
                ptr = x;
                x = x->parent;
            }
            ptr = x;
        }

        return *this;
    }

    RBTreeConstIterator operator--(int) {
        RBTreeConstIterator tmp = *this;
        --*this;

        return tmp;
    }

    bool operator==(const RBTreeConstIterator& rhs) const {
        return ptr == rhs.ptr;
    }

    bool operator!=(const RBTreeConstIterator& rhs) const {
        return !(ptr == rhs.ptr);
    }

}; // RBTreeConstIterator

template <typename T>
struct RBTreeIterator : RBTreeConstIterator<T> {
    using iterator_category = bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    using Ptr = RBTNode<T>*;
    using Base = RBTreeConstIterator<T>;

    RBTreeIterator() : Base() {
    }
    RBTreeIterator(Ptr x) : Base(x) {
    }
    RBTreeIterator(const RBTreeIterator& rhs) : Base(rhs.ptr) {
    }

    reference operator*() const {
        return const_cast<reference>(Base::operator*());
    }

    pointer operator->() const {
        return pointer_traits<pointer>::pointer_to(**this);
    }

    RBTreeIterator& operator++() {
        ++*static_cast<Base*>(this);
        return *this;
    }

    RBTreeIterator operator++(int) {
        RBTreeIterator tmp = *this;
        ++*this;
        return tmp;
    }

    RBTreeIterator& operator--() {
        --*static_cast<Base*>(this);
        return *this;
    }

    RBTreeIterator operator--(int) {
        RBTreeIterator tmp = *this;
        --*this;
        return tmp;
    }
}; // RBTreeIterator

template <typename T, typename Compare, typename Alloc>
class RBTreeBase {
public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = typename Alloc::size_type;
    using difference_type = typename Alloc::difference_type;

private:
    using AlTraits = allocator_traits<Alloc>;
    using Node = RBTNode<T>;
    using NodePtr = RBTNode<T>*;
    using AlNode = typename AlTraits::template rebind_alloc<Node>;
    using AlNodeTraits = allocator_traits<AlNode>;

protected:
    NodePtr header;
    size_type mCount;
    AlNode alloc;
    Compare compare;

public:
    RBTreeBase() : mCount(0), alloc(), compare() {
        createHeaderNode();
    }

    template <
        typename Any_alloc,
        typename = enable_if_t<!is_same<decay_t<Any_alloc>, RBTreeBase>::value>>
    RBTreeBase(const Compare& cmp, Any_alloc&& anyAlloc)
        : mCount(0), alloc(tiny_stl::forward<Any_alloc>(anyAlloc)),
          compare(cmp) {
        createHeaderNode();
    }

    ~RBTreeBase() {
        alloc.deallocate(header, 1);
    }

private:
    void createHeaderNode() {
        try {
            header = alloc.allocate(1);
            header->left = header;
            header->right = header;
            header->parent = header;
            header->isNil = 1;
            header->color = Color::BLACK;
        } catch (...) {
            alloc.deallocate(header, 1);
            throw;
        }
    }
}; // RBTreeBase

template <typename T, typename Compare, typename Alloc, bool isMap>
class RBTree : public RBTreeBase<T, Compare, Alloc> {
public:
    using key_type = typename AssociatedTypeHelper<T, isMap>::key_type;
    using mapped_type = typename AssociatedTypeHelper<T, isMap>::mapped_type;
    using value_type = T;
    using key_compare = Compare; // if it is map, compare pair
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    using AlTraits = allocator_traits<Alloc>;
    using Node = RBTNode<value_type>;
    using NodePtr = RBTNode<value_type>*;
    using AlNode = typename AlTraits::template rebind_alloc<Node>;
    using AlNodeTraits = allocator_traits<AlNode>;
    using Base = RBTreeBase<value_type, Compare, Alloc>;

    using iterator = RBTreeIterator<value_type>;
    using const_iterator = RBTreeConstIterator<value_type>;
    using reverse_iterator = tiny_stl::reverse_iterator<iterator>;
    using const_reverse_iterator = tiny_stl::reverse_iterator<const_iterator>;

private:
    template <typename... Args>
    NodePtr allocAndConstruct(Args&&... args) {
        NodePtr p = this->alloc.allocate(1);

        p->color = Color::RED;
        p->isNil = 0;
        p->parent = this->header;
        p->left = this->header;
        p->right = this->header;

        try {
            this->alloc.construct(tiny_stl::addressof(p->value),
                                  tiny_stl::forward<Args>(args)...);
        } catch (...) {
            this->alloc.deallocate(p, 1);
            throw;
        }

        return p;
    }

    void destroyAndFree(NodePtr p) {
        if (p != nullptr) {
            this->alloc.destroy(tiny_stl::addressof(p->value));
            this->alloc.deallocate(p, 1);
        }
    }

    NodePtr& getRoot() const noexcept {
        return this->header->parent;
    }

    // map
    const key_type& getKeyNode(NodePtr ptr, true_type) const {
        return ptr->value.first;
    }

    // set
    const key_type& getKeyNode(NodePtr ptr, false_type) const {
        return ptr->value;
    }

    const key_type& get_key(NodePtr ptr) const {
        return getKeyNode(ptr, tiny_stl::bool_constant<isMap>{});
    }

    // map
    const key_type& getKeyValue(const T& val, true_type) const {
        return val.first;
    }

    // set
    const key_type& getKeyValue(const T& val, false_type) const {
        return val;
    }

    const key_type& getKeyFromValue(const T& val) const {
        return getKeyValue(val, tiny_stl::bool_constant<isMap>{});
    }

    template <typename K>
    NodePtr lowBoundAux(const K& val) const {
        NodePtr pos = this->header;
        NodePtr p = pos->parent;

        while (!p->isNil) {
            if (this->compare(get_key(p), val)) {
                p = p->right;
            } else {
                pos = p;
                p = p->left;
            }
        }

        return pos;
    }

    template <typename K>
    NodePtr uppBoundAux(const K& val) const {
        NodePtr pos = this->header;
        NodePtr p = pos->parent;

        while (!p->isNil) {
            if (!this->compare(val, get_key(p))) {
                p = p->right;
            } else {
                pos = p;
                p = p->left;
            }
        }

        return pos;
    }

    NodePtr copyNodes(NodePtr rhsRoot, NodePtr thisPos) {
        NodePtr newheader = this->header;

        if (!rhsRoot->isNil) {
            NodePtr p = this->alloc.allocate(1);
            p->color = rhsRoot->color;
            p->isNil = 0;
            p->parent = thisPos;
            try {
                p->value = rhsRoot->value;
            } catch (...) {
                this->alloc.deallocate(p, 1);
                throw;
            }

            if (newheader->isNil)
                newheader = p;

            p->left = copyNodes(rhsRoot->left, p);
            p->right = copyNodes(rhsRoot->right, p);
        }

        return newheader;
    }

    void copyAux(const RBTree& rhs) {
        getRoot() = copyNodes(rhs.getRoot(), this->header);
        this->mCount = rhs.mCount;

        if (!getRoot()->isNil) {
            this->header->left = rbTreeMinValue(getRoot());
            this->header->right = rbTreeMaxValue(getRoot());
        } else {
            this->header->left = this->header;
            this->header->right = this->header;
        }
    }

    void moveAux(RBTree&& rhs) {
        tiny_stl::swapADL(this->compare, rhs.compare);
        tiny_stl::swapADL(this->header, rhs.header);
        tiny_stl::swapADL(this->mCount, rhs.mCount);
    }

    void rbTreeFixupForInsert(NodePtr& root, NodePtr z) {
        while (z->parent->color == Color::RED) { // parent is red
            // if parent is grandfather's left child
            if (z->parent == z->parent->parent->left) {
                NodePtr y = z->parent->parent->right; // y is z's uncle

                if (y->color == Color::RED) { // case 1, z's uncle is red
                    z->parent->color = Color::BLACK;
                    y->color = Color::BLACK;
                    z->parent->parent->color = Color::RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->right) {
                        // case 2, z is parent's right child
                        z = z->parent;
                        rbTreeLeftRotate(root, z);
                    }
                    z->parent->color =
                        Color::BLACK; // case 3, z is parent's left child
                    z->parent->parent->color = Color::RED;
                    rbTreeRightRotate(root, z->parent->parent);
                }
            } else { // parent is grandfather's right
                NodePtr y = z->parent->parent->left; // y is z's uncle

                if (y->color == Color::RED) { // case 1, z's uncle is red
                    z->parent->color = Color::BLACK;
                    y->color = Color::BLACK;
                    z->parent->parent->color = Color::RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->left) {
                        // case 2, z is parent's left child
                        z = z->parent;
                        rbTreeRightRotate(root, z);
                    }
                    // case 3, z is parent's left child
                    z->parent->color = Color::BLACK;
                    z->parent->parent->color = Color::RED;
                    rbTreeLeftRotate(root, z->parent->parent);
                }
            }
        }

        root->color = Color::BLACK;
    }

    void rbTreeFixupForErase(NodePtr& root, NodePtr x) {
        while (x != root && x->color == Color::BLACK) {
            if (x == x->parent->left) {
                NodePtr w = x->parent->right;
                if (w->color == Color::RED) { // case 1, x's brother w is red
                    w->color = Color::BLACK;
                    x->parent->color = Color::RED;
                    rbTreeLeftRotate(root, x->parent);
                    w = x->parent->right;
                }
                if (w->left->color == Color::BLACK &&
                    w->right->color == Color::BLACK) {
                    // case 2, w's left and right child is black
                    w->color = Color::RED;
                    x = x->parent;
                } else { // case 3, w's right child is black
                    if (w->right->color == Color::BLACK) {
                        w->left->color = Color::BLACK;
                        w->color = Color::RED;
                        rbTreeRightRotate(root, w);
                        w = x->parent->right;
                    }
                    w->color = x->parent->color;
                    x->parent->color = Color::BLACK;
                    w->right->color = Color::BLACK;
                    rbTreeLeftRotate(root, x->parent);
                    x = root;
                }
            } else {
                NodePtr w = x->parent->left;
                if (w->color == Color::RED) { // case 1
                    w->color = Color::BLACK;
                    x->parent->color = Color::RED;
                    rbTreeRightRotate(root, x->parent);
                    w = x->parent->left;
                }
                if (w->left->color == Color::BLACK &&
                    w->right->color == Color::BLACK) { // case 2
                    w->color = Color::RED;
                    x = x->parent;
                } else {
                    if (w->left->color == Color::BLACK) { // case 3
                        w->right->color = Color::BLACK;
                        w->color = Color::RED;
                        rbTreeLeftRotate(root, w);
                        w = x->parent->left;
                    }
                    // case 4
                    w->color = x->parent->color;
                    x->parent->color = Color::BLACK;
                    w->left->color = Color::BLACK;
                    rbTreeRightRotate(root, x->parent);
                    x = root;
                }
            }
        }

        x->color = Color::BLACK;
        this->header->color = Color::BLACK;
    }

public:
    RBTree() : Base() {
    }

    RBTree(const Compare& cmp) : Base(cmp) {
    }

    RBTree(const Compare& cmp, const allocator_type& alloc) : Base(cmp, alloc) {
    }

    template <typename Any_alloc>
    RBTree(const RBTree& rhs, Any_alloc&& alloc)
        : Base(rhs.compare, tiny_stl::forward<Any_alloc>(alloc)) {
        copyAux(rhs);
    }

    RBTree(RBTree&& rhs) noexcept : Base(rhs.compare, rhs.alloc) {
        moveAux(tiny_stl::move(rhs));
    }

    RBTree(RBTree&& rhs, const Alloc& alloc) : Base(rhs.compare, alloc) {
        moveAux(tiny_stl::move(rhs));
    }

    RBTree& operator=(const RBTree& rhs) {
        // Non-standard, user allocator may be wrong
        clear();
        copyAux(rhs);

        return *this;
    }

    RBTree& operator=(RBTree&& rhs) {
        // Non-standard, user allocator may be wrong
        clear();
        moveAux(tiny_stl::move(rhs));

        return *this;
    }

    ~RBTree() {
        clear();
    }

    allocator_type get_allocator() const noexcept {
        return this->alloc;
    }

    size_type size() const noexcept {
        return this->mCount;
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    size_type max_size() const noexcept {
        return AlNodeTraits::max_size(this->alloc);
    }

    iterator lower_bound(const key_type& val) {
        return iterator(lowBoundAux(val));
    }

    const_iterator lower_bound(const key_type& val) const {
        return const_iterator(lowBoundAux(val));
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    iterator lower_bound(const K& val) {
        return iterator(lowBoundAux(val));
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    const_iterator lower_bound(const K& val) const {
        return iterator(lowBoundAux(val));
    }

    iterator upper_bound(const key_type& val) {
        return iterator(uppBoundAux(val));
    }

    const_iterator upper_bound(const key_type& val) const {
        return const_iterator(uppBoundAux(val));
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    iterator upper_bound(const K& val) {
        return iterator(uppBoundAux(val));
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    const_iterator upper_bound(const K& val) const {
        return const_iterator(uppBoundAux(val));
    }

    pair<iterator, iterator> equal_range(const key_type& key) {
        return {lower_bound(key), upper_bound(key)};
    }

    pair<const_iterator, const_iterator>
    equal_range(const key_type& key) const {
        return {lower_bound(key), upper_bound(key)};
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    pair<iterator, iterator> equal_range(const K& key) {
        return {lower_bound(key), upper_bound(key)};
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    pair<const_iterator, const_iterator> equal_range(const K& key) const {
        return {lower_bound(key), upper_bound(key)};
    }

    size_type count(const key_type& key) const {
        pair<const_iterator, const_iterator> range = equal_range(key);
        return tiny_stl::distance(range.first, range.second);
    }

    template <typename K>
    size_type count(const K& key) const {
        pair<const_iterator, const_iterator> range = equal_range(key);
        return tiny_stl::distance(range.first, range.second);
    }

    iterator find(const key_type& val) {
        iterator pos = lower_bound(val);
        return (pos == end() || this->compare(val, get_key(pos.ptr))) ? end()
                                                                      : pos;
    }

    const_iterator find(const key_type& val) const {
        const_iterator pos = lower_bound(val);
        return (pos == end() || this->compare(val, get_key(pos.ptr))) ? end()
                                                                      : pos;
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    iterator find(const K& val) {
        iterator pos = lower_bound(val);
        return (pos == end() || this->compare(val, get_key(pos.ptr))) ? end()
                                                                      : pos;
    }

    template <typename K, typename Cmp = Compare,
              typename = typename Cmp::is_transparent>
    const_iterator find(const K& val) const {
        iterator pos = lower_bound(val);
        return (pos == end() || this->compare(val, get_key(pos.ptr))) ? end()
                                                                      : pos;
    }

private:
    iterator insertAux(NodePtr z) {
        NodePtr x = getRoot();
        NodePtr y = this->header;

        if (y->left->isNil || this->compare(get_key(z), get_key(y->left)))
            y->left = z;

        if (y->right->isNil || !this->compare(get_key(z), get_key(y->right)))
            y->right = z;

        while (!x->isNil) {
            y = x;
            if (this->compare(get_key(z), get_key(x)))
                x = x->left;
            else
                x = x->right;
        }

        z->parent = y;

        if (y->isNil) {
            getRoot() = z;
            this->header->left = z;
            this->header->right = z;
        } else if (this->compare(get_key(z), get_key(y))) {
            y->left = z;
            if (this->compare(get_key(z), get_key(this->header->left)))
                this->header->left = z; // z.key < min_value
        } else {
            y->right = z;
            if (!this->compare(get_key(z), get_key(this->header->right)))
                this->header->right = z; // z.key >= max_value
        }

        rbTreeFixupForInsert(getRoot(), z);

        ++this->mCount;

        return iterator(z);
    }

    template <typename Value>
    iterator insertEqualAux(Value&& val) {
        NodePtr z = allocAndConstruct(tiny_stl::forward<Value>(val));
        return insertAux(z);
    }

    template <typename Value>
    pair<iterator, bool> insertUniqueAux(Value&& val) {
        T value(tiny_stl::forward<Value>(val));
        iterator pos = find(getKeyFromValue(val));

        if (pos != end()) {
            return make_pair(pos, false);
        }

        NodePtr z = allocAndConstruct(tiny_stl::move(value));
        return make_pair(insertAux(z), true);
    }

protected:
    iterator insert_equal(const value_type& val) {
        return insertEqualAux(val);
    }

    iterator insert_equal(value_type&& val) {
        return insertEqualAux(tiny_stl::move(val));
    }

    template <typename InIter>
    void insert_equal(InIter first, InIter last) {
        for (; first != last; ++first)
            insertEqualAux(*first);
    }

    pair<iterator, bool> insert_unique(const value_type& val) {
        return insertUniqueAux(val);
    }

    pair<iterator, bool> insert_unique(value_type&& val) {
        return insertUniqueAux(tiny_stl::move(val));
    }

    template <typename InIter>
    void insert_unique(InIter first, InIter last) {
        for (; first != last; ++first)
            insertUniqueAux(*first);
    }

    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        return insertEqualAux(tiny_stl::forward<Args>(args)...);
    }

    template <typename... Args>
    pair<iterator, bool> emplace_unique(Args&&... args) {
        return insertUniqueAux(tiny_stl::forward<Args>(args)...);
    }

private:
    // transplant v to the location of u
    inline void transplantForErase(NodePtr& root, NodePtr u, NodePtr v) {
        if (u->parent->isNil) // u is root
            root = v;
        else if (u == u->parent->left) // u is left child
            u->parent->left = v;
        else // u is right child
            u->parent->right = v;

        v->parent = u->parent;
    }

    // erase node z
    void eraseAux(NodePtr root, NodePtr z) {
        NodePtr y = z;
        NodePtr x = nullptr;

        Color yOriginColor = y->color;

        if (z == this->header->left)
            this->header->left = (++iterator(z)).ptr;

        if (z == this->header->right)
            this->header->right = (--iterator(z)).ptr;

        if (z->left->isNil) { // z has not left child
            x = z->right;
            transplantForErase(getRoot(), z, z->right);
        } else if (z->right->isNil) { // z has not right child
            x = z->left;
            transplantForErase(getRoot(), z, z->left);
        } else { // z has left and right child
            y = rbTreeMinValue(z->right);
            yOriginColor = y->color;
            x = y->right;

            if (y->parent == z) {
                x->parent = y;
            } else {
                transplantForErase(getRoot(), y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }

            transplantForErase(getRoot(), z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }

        if (yOriginColor == Color::BLACK)
            rbTreeFixupForErase(root, x);

        destroyAndFree(z);
        --this->mCount;

        this->header->parent = root;
        root->parent = this->header;
    }

public:
    iterator erase(const_iterator pos) {
        NodePtr z = pos.ptr;

        ++pos;
        eraseAux(getRoot(), z);
        return iterator(pos.ptr);
    }

    iterator erase(const_iterator first, const_iterator last) {
        if (first == begin() && last == end()) {
            clear();
            return end();
        }

        while (first != last)
            erase(first++);

        return iterator(first.ptr);
    }

    size_type erase(const key_type& key) {
        auto ppos = equal_range(key);
        size_type num = tiny_stl::distance(ppos.first, ppos.second);

        erase(ppos.first, ppos.second);

        return num;
    }

public:
    iterator begin() noexcept {
        return iterator(this->header->left);
    }

    const_iterator begin() const noexcept {
        return const_iterator(this->header->left);
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return iterator(this->header);
    }

    const_iterator end() const noexcept {
        return const_iterator(this->header);
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
    void clearAux(NodePtr root) {
        for (NodePtr p = root; !p->isNil; root = p) {
            clearAux(p->right);
            p = p->left;
            AlNodeTraits::destroy(this->alloc,
                                  tiny_stl::addressof(root->value));
            this->alloc.deallocate(root, 1);
        }
    }

public:
    void clear() {
        clearAux(getRoot());
        this->header->left = this->header;
        this->header->right = this->header;
        this->header->parent = this->header;
        this->mCount = 0;
    }

    void swap(RBTree& rhs) noexcept(AlTraits::is_always_equal::value&&
                                        is_nothrow_swappable<Compare>::value) {
        assert(this->alloc == rhs.alloc);

        if (allocator_traits<Alloc>::propagate_on_container_swap::value)
            tiny_stl::swapAlloc(this->alloc, rhs.alloc);

        tiny_stl::swapADL(this->header, rhs.header);
        tiny_stl::swapADL(this->compare, rhs.compare);
        tiny_stl::swapADL(this->mCount, rhs.mCount);
    }

}; // RBTree

template <typename T, typename Compare, typename Alloc, bool isMap>
bool operator==(const RBTree<T, Compare, Alloc, isMap>& lhs,
                const RBTree<T, Compare, Alloc, isMap>& rhs) {
    return lhs.size() == rhs.size() &&
           tiny_stl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Compare, typename Alloc, bool isMap>
bool operator!=(const RBTree<T, Compare, Alloc, isMap>& lhs,
                const RBTree<T, Compare, Alloc, isMap>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Compare, typename Alloc, bool isMap>
bool operator<(const RBTree<T, Compare, Alloc, isMap>& lhs,
               const RBTree<T, Compare, Alloc, isMap>& rhs) {
    return tiny_stl::lexicographical_compare(lhs.begin(), lhs.end(),
                                             rhs.begin(), rhs.end());
}

template <typename T, typename Compare, typename Alloc, bool isMap>
bool operator>(const RBTree<T, Compare, Alloc, isMap>& lhs,
               const RBTree<T, Compare, Alloc, isMap>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Compare, typename Alloc, bool isMap>
bool operator<=(const RBTree<T, Compare, Alloc, isMap>& lhs,
                const RBTree<T, Compare, Alloc, isMap>& rhs) {
    return !(rhs < lhs);
}

template <typename T, typename Compare, typename Alloc, bool isMap>
bool operator>=(const RBTree<T, Compare, Alloc, isMap>& lhs,
                const RBTree<T, Compare, Alloc, isMap>& rhs) {
    return !(lhs < rhs);
}

} // namespace tiny_stl
