#include "set"
#include <stdlib.h>

namespace std {

template <typename T>
set<T>::set() : root_(nullptr), size_(0) {
    nil_ = new set_node<T>(T(), Color::BLACK, true);
    root_ = nil_;
}

template <typename T>
set<T>::set(const set<T>& src) : root_(nullptr), size_(0) {
    nil_ = new set_node<T>(T(), Color::BLACK, true);
    root_ = nil_;
    if (src.root_ != src.nil_) {
        root_ = copy_tree(src.root_, nullptr);
    }
}

template <typename T>
set<T>::~set() {
    delete_tree(root_);
    delete nil_;
}

template <typename T>
set<T>& set<T>::operator=(const set& rhs) {
    if (this != &rhs) {
        clear();
        if (rhs.root_ != rhs.nil_) {
            root_ = copy_tree(rhs.root_, nullptr);
            size_ = rhs.size_;
        }
    }
    return *this;
}

template <typename T>
set_node<T>* set<T>::copy_tree(set_node<T>* node, set_node<T>* parent) {
    if (node == nullptr || node->is_nil) {
        return nil_;
    }
    set_node<T>* new_node = new set_node<T>(node->key, node->color);
    new_node->parent = parent;
    new_node->left = copy_tree(node->left, new_node);
    new_node->right = copy_tree(node->right, new_node);
    return new_node;
}

template <typename T>
typename set<T>::iterator set<T>::begin() {
    if (root_ == nil_) {
        return iterator(nullptr);
    }
    set_node<T>* min = minimum(root_);
    return iterator(&min->key);
}

template <typename T>
typename set<T>::const_iterator set<T>::begin() const {
    if (root_ == nil_) {
        return const_iterator(nullptr);
    }
    set_node<T>* min = minimum(root_);
    return const_iterator(&min->key);
}

template <typename T>
typename set<T>::iterator set<T>::end() {
    return iterator(nullptr);
}

template <typename T>
typename set<T>::const_iterator set<T>::end() const {
    return const_iterator(nullptr);
}

template <typename T>
size_t set<T>::size() const {
    return size_;
}

template <typename T>
bool set<T>::empty() const {
    return size_ == 0;
}

template <typename T>
void set<T>::clear() {
    delete_tree(root_);
    root_ = nil_;
    size_ = 0;
}

template <typename T>
void set<T>::delete_tree(set_node<T>* x) {
    if (x == nullptr || x == nil_) return;
    delete_tree(x->left);
    delete_tree(x->right);
    if (!x->is_nil) {
        delete x;
    }
}

template <typename T>
typename set<T>::iterator set<T>::find(const T& val) {
    set_node<T>* current = root_;
    while (current != nil_) {
        if (current->key == val) {
            return iterator(&current->key);
        } else if (val < current->key) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    return end();
}

template <typename T>
typename set<T>::const_iterator set<T>::find(const T& val) const {
    set_node<T>* current = root_;
    while (current != nil_) {
        if (current->key == val) {
            return const_iterator(&current->key);
        } else if (val < current->key) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    return end();
}

template <typename T>
typename set<T>::iterator set<T>::insert(const T& val) {
    set_node<T>* z = new set_node<T>(val);
    set_node<T>* y = nil_;
    set_node<T>* x = root_;

    while (x != nil_) {
        y = x;
        if (val < x->key) {
            x = x->left;
        } else if (x->key < val) {
            x = x->right;
        } else {
            delete z;
            return iterator(&x->key);
        }
    }

    z->parent = y;
    if (y == nil_) {
        root_ = z;
    } else if (val < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }

    z->left = nil_;
    z->right = nil_;
    z->color = Color::RED;
    size_++;

    insert_fixup(z);

    return iterator(&z->key);
}

template <typename T>
void set<T>::left_rotate(set_node<T>* x) {
    set_node<T>* y = x->right;
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

template <typename T>
void set<T>::right_rotate(set_node<T>* x) {
    set_node<T>* y = x->left;
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

template <typename T>
void set<T>::insert_fixup(set_node<T>* z) {
    while (z->parent != nil_ && z->parent->color == Color::RED) {
        if (z->parent == z->parent->parent->left) {
            set_node<T>* y = z->parent->parent->right;
            if (y->color == Color::RED) {
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(z);
                }
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                right_rotate(z->parent->parent);
            }
        } else {
            set_node<T>* y = z->parent->parent->left;
            if (y->color == Color::RED) {
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(z);
                }
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                left_rotate(z->parent->parent);
            }
        }
    }
    root_->color = Color::BLACK;
}

template <typename T>
size_t set<T>::erase(const T& val) {
    set_node<T>* z = root_;
    while (z != nil_) {
        if (z->key == val) {
            break;
        } else if (val < z->key) {
            z = z->left;
        } else {
            z = z->right;
        }
    }

    if (z == nil_) {
        return 0;
    }

    set_node<T>* y = z;
    set_node<T>* x;
    Color y_original_color = y->color;

    if (z->left == nil_) {
        x = z->right;
        transplant(z, z->right);
    } else if (z->right == nil_) {
        x = z->left;
        transplant(z, z->left);
    } else {
        y = minimum(z->right);
        y_original_color = y->color;
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
        y->color = z->color;
    }

    if (y_original_color == Color::BLACK) {
        erase_fixup(x);
    }

    delete z;
    size_--;
    return 1;
}

template <typename T>
void set<T>::transplant(set_node<T>* u, set_node<T>* v) {
    if (u->parent == nil_) {
        root_ = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    v->parent = u->parent;
}

template <typename T>
set_node<T>* set<T>::minimum(set_node<T>* x) const {
    while (x->left != nil_) {
        x = x->left;
    }
    return x;
}

template <typename T>
set_node<T>* set<T>::maximum(set_node<T>* x) const {
    while (x->right != nil_) {
        x = x->right;
    }
    return x;
}

template <typename T>
void set<T>::erase_fixup(set_node<T>* x) {
    while (x != root_ && x->color == Color::BLACK) {
        if (x == x->parent->left) {
            set_node<T>* w = x->parent->right;
            if (w->color == Color::RED) {
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                left_rotate(x->parent);
                w = x->parent->right;
            }
            if (w->left->color == Color::BLACK && w->right->color == Color::BLACK) {
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->right->color == Color::BLACK) {
                    w->left->color = Color::BLACK;
                    w->color = Color::RED;
                    right_rotate(w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                w->right->color = Color::BLACK;
                left_rotate(x->parent);
                x = root_;
            }
        } else {
            set_node<T>* w = x->parent->left;
            if (w->color == Color::RED) {
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                right_rotate(x->parent);
                w = x->parent->left;
            }
            if (w->right->color == Color::BLACK && w->left->color == Color::BLACK) {
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (w->left->color == Color::BLACK) {
                    w->right->color = Color::BLACK;
                    w->color = Color::RED;
                    left_rotate(w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                w->left->color = Color::BLACK;
                right_rotate(x->parent);
                x = root_;
            }
        }
    }
    x->color = Color::BLACK;
}

template <typename T>
void set<T>::swap(set& other) {
    set_node<T>* temp_root = root_;
    root_ = other.root_;
    other.root_ = temp_root;

    size_t temp_size = size_;
    size_ = other.size_;
    other.size_ = temp_size;
}

// Explicit template instantiations
template class set<int>;

}