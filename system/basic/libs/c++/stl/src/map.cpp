#include "map"
#include <stdlib.h>

namespace std {

template <typename Key, typename T>
map<Key, T>::map() : root_(nullptr), size_(0) {
    nil_ = new map_node<value_type>(value_type(), false, true);
    root_ = nil_;
}

template <typename Key, typename T>
map<Key, T>::map(const map<Key, T>& src) : root_(nullptr), size_(0) {
    nil_ = new map_node<value_type>(value_type(), false, true);
    root_ = nil_;
    if (src.root_ != src.nil_) {
        root_ = copy_tree(src.root_, nullptr);
        size_ = src.size_;
    }
}

template <typename Key, typename T>
map<Key, T>::~map() {
    delete_tree(root_);
    delete nil_;
}

template <typename Key, typename T>
map<Key, T>& map<Key, T>::operator=(const map& rhs) {
    if (this != &rhs) {
        clear();
        if (rhs.root_ != rhs.nil_) {
            root_ = copy_tree(rhs.root_, nullptr);
            size_ = rhs.size_;
        }
    }
    return *this;
}

template <typename Key, typename T>
map_node<typename map<Key, T>::value_type>* map<Key, T>::copy_tree(map_node<typename map<Key, T>::value_type>* node, map_node<typename map<Key, T>::value_type>* parent) {
    if (node == nullptr || node->is_nil) {
        return nil_;
    }
    map_node<typename map<Key, T>::value_type>* new_node = new map_node<typename map<Key, T>::value_type>(node->key, node->is_red);
    new_node->parent = parent;
    new_node->left = copy_tree(node->left, new_node);
    new_node->right = copy_tree(node->right, new_node);
    return new_node;
}

template <typename Key, typename T>
typename map<Key, T>::iterator map<Key, T>::begin() {
    if (root_ == nil_) {
        return iterator(nullptr);
    }
    map_node<value_type>* min = minimum(root_);
    return iterator(min);
}

template <typename Key, typename T>
typename map<Key, T>::const_iterator map<Key, T>::begin() const {
    if (root_ == nil_) {
        return const_iterator(nullptr);
    }
    map_node<value_type>* min = minimum(root_);
    return const_iterator(min);
}

template <typename Key, typename T>
typename map<Key, T>::iterator map<Key, T>::end() {
    return iterator(nullptr);
}

template <typename Key, typename T>
typename map<Key, T>::const_iterator map<Key, T>::end() const {
    return const_iterator(nullptr);
}

template <typename Key, typename T>
size_t map<Key, T>::size() const {
    return size_;
}

template <typename Key, typename T>
bool map<Key, T>::empty() const {
    return size_ == 0;
}

template <typename Key, typename T>
void map<Key, T>::clear() {
    delete_tree(root_);
    root_ = nil_;
    size_ = 0;
}

template <typename Key, typename T>
void map<Key, T>::delete_tree(map_node<value_type>* x) {
    if (x == nullptr || x->is_nil) return;
    delete_tree(x->left);
    delete_tree(x->right);
    if (!x->is_nil) {
        delete x;
    }
}

template <typename Key, typename T>
typename map<Key, T>::iterator map<Key, T>::find(const Key& key) {
    map_node<value_type>* current = root_;
    while (current != nil_) {
        if (current->key.first == key) {
            return iterator(current);
        } else if (key < current->key.first) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    return end();
}

template <typename Key, typename T>
typename map<Key, T>::const_iterator map<Key, T>::find(const Key& key) const {
    map_node<value_type>* current = root_;
    while (current != nil_) {
        if (current->key.first == key) {
            return const_iterator(current);
        } else if (key < current->key.first) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    return end();
}

template <typename Key, typename T>
typename map<Key, T>::iterator map<Key, T>::insert(const value_type& val) {
    map_node<value_type>* z = new map_node<value_type>(val, true);
    map_node<value_type>* y = nil_;
    map_node<value_type>* x = root_;

    while (x != nil_) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        } else {
            x = x->right;
        }
    }

    z->parent = y;
    if (y == nil_) {
        root_ = z;
    } else if (z->key < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }

    z->left = nil_;
    z->right = nil_;
    size_++;

    insert_fixup(z);

    return iterator(z);
}

template <typename Key, typename T>
T& map<Key, T>::operator[](const Key& key) {
    map_node<value_type>* current = root_;
    while (current != nil_) {
        if (current->key.first == key) {
            return current->key.second;
        } else if (key < current->key.first) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    value_type val(key, T());
    insert(val);
    iterator it = find(key);
    return it->second;
}

template <typename Key, typename T>
void map<Key, T>::left_rotate(map_node<value_type>* x) {
    map_node<value_type>* y = x->right;
    x->right = y->left;
    if (y->left != nil_) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == nil_) {
        root_ = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

template <typename Key, typename T>
void map<Key, T>::right_rotate(map_node<value_type>* x) {
    map_node<value_type>* y = x->left;
    x->left = y->right;
    if (y->right != nil_) {
        y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == nil_) {
        root_ = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

template <typename Key, typename T>
void map<Key, T>::insert_fixup(map_node<value_type>* z) {
    while (z->parent != nil_ && z->parent->is_red) {
        if (z->parent == z->parent->parent->left) {
            map_node<value_type>* y = z->parent->parent->right;
            if (y->is_red) {
                z->parent->is_red = false;
                y->is_red = false;
                z->parent->parent->is_red = true;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(z);
                }
                z->parent->is_red = false;
                z->parent->parent->is_red = true;
                right_rotate(z->parent->parent);
            }
        } else {
            map_node<value_type>* y = z->parent->parent->left;
            if (y->is_red) {
                z->parent->is_red = false;
                y->is_red = false;
                z->parent->parent->is_red = true;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(z);
                }
                z->parent->is_red = false;
                z->parent->parent->is_red = true;
                left_rotate(z->parent->parent);
            }
        }
    }
    root_->is_red = false;
}

template <typename Key, typename T>
size_t map<Key, T>::erase(const Key& key) {
    map_node<value_type>* z = root_;
    while (z != nil_) {
        if (z->key.first == key) {
            break;
        } else if (key < z->key.first) {
            z = z->left;
        } else {
            z = z->right;
        }
    }

    if (z == nil_) {
        return 0;
    }

    map_node<value_type>* y = z;
    map_node<value_type>* x;
    bool y_original_is_red = y->is_red;

    if (z->left == nil_) {
        x = z->right;
        transplant(z, z->right);
    } else if (z->right == nil_) {
        x = z->left;
        transplant(z, z->left);
    } else {
        y = minimum(z->right);
        y_original_is_red = y->is_red;
        x = y->right;
        if (y->parent == z) {
            x->parent = y;
        } else {
            transplant(y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(z, y);
        y->left = z->left;
        y->left->parent = y;
        y->is_red = z->is_red;
    }

    if (!y_original_is_red) {
        erase_fixup(x);
    }

    delete z;
    size_--;
    return 1;
}

template <typename Key, typename T>
void map<Key, T>::transplant(map_node<value_type>* u, map_node<value_type>* v) {
    if (u->parent == nil_) {
        root_ = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    v->parent = u->parent;
}

template <typename Key, typename T>
map_node<typename map<Key, T>::value_type>* map<Key, T>::minimum(map_node<typename map<Key, T>::value_type>* x) const {
    while (x->left != nil_) {
        x = x->left;
    }
    return x;
}

template <typename Key, typename T>
void map<Key, T>::erase_fixup(map_node<value_type>* x) {
    while (x != root_ && !x->is_red) {
        if (x == x->parent->left) {
            map_node<value_type>* w = x->parent->right;
            if (w->is_red) {
                w->is_red = false;
                x->parent->is_red = true;
                left_rotate(x->parent);
                w = x->parent->right;
            }
            if (!w->left->is_red && !w->right->is_red) {
                w->is_red = true;
                x = x->parent;
            } else {
                if (!w->right->is_red) {
                    w->left->is_red = false;
                    w->is_red = true;
                    right_rotate(w);
                    w = x->parent->right;
                }
                w->is_red = x->parent->is_red;
                x->parent->is_red = false;
                w->right->is_red = false;
                left_rotate(x->parent);
                x = root_;
            }
        } else {
            map_node<value_type>* w = x->parent->left;
            if (w->is_red) {
                w->is_red = false;
                x->parent->is_red = true;
                right_rotate(x->parent);
                w = x->parent->left;
            }
            if (!w->right->is_red && !w->left->is_red) {
                w->is_red = true;
                x = x->parent;
            } else {
                if (!w->left->is_red) {
                    w->right->is_red = false;
                    w->is_red = true;
                    left_rotate(w);
                    w = x->parent->left;
                }
                w->is_red = x->parent->is_red;
                x->parent->is_red = false;
                w->left->is_red = false;
                right_rotate(x->parent);
                x = root_;
            }
        }
    }
    x->is_red = false;
}

template <typename Key, typename T>
void map<Key, T>::swap(map& other) {
    map_node<value_type>* temp_root = root_;
    root_ = other.root_;
    other.root_ = temp_root;

    size_t temp_size = size_;
    size_ = other.size_;
    other.size_ = temp_size;
}

// Explicit template instantiations
template class map<int, int>;

}