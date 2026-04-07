#include "list"
#include <stdlib.h>

namespace std {

template <typename T>
list<T>::list() : head_(nullptr), tail_(nullptr), size_(0) {
}

template <typename T>
list<T>::list(size_t n, const T& val) : head_(nullptr), tail_(nullptr), size_(0) {
    for (size_t i = 0; i < n; i++) {
        push_back(val);
    }
}

template <typename T>
list<T>::list(const list<T>& src) : head_(nullptr), tail_(nullptr), size_(0) {
    Node* current = src.head_;
    while (current != nullptr) {
        push_back(current->data);
        current = current->next;
    }
}

template <typename T>
list<T>::~list() {
    clear();
}

template <typename T>
list<T>& list<T>::operator=(const list& rhs) {
    if (this != &rhs) {
        clear();
        Node* current = rhs.head_;
        while (current != nullptr) {
            push_back(current->data);
            current = current->next;
        }
    }
    return *this;
}

template <typename T>
typename list<T>::iterator list<T>::begin() {
    return iterator(head_);
}

template <typename T>
typename list<T>::const_iterator list<T>::begin() const {
    return const_iterator(head_);
}

template <typename T>
typename list<T>::iterator list<T>::end() {
    return iterator(tail_ ? tail_->next : nullptr);
}

template <typename T>
typename list<T>::const_iterator list<T>::end() const {
    return const_iterator(tail_ ? tail_->next : nullptr);
}

template <typename T>
T& list<T>::front() {
    return head_->data;
}

template <typename T>
const T& list<T>::front() const {
    return head_->data;
}

template <typename T>
T& list<T>::back() {
    return tail_->data;
}

template <typename T>
const T& list<T>::back() const {
    return tail_->data;
}

template <typename T>
size_t list<T>::size() const {
    return size_;
}

template <typename T>
bool list<T>::empty() const {
    return size_ == 0;
}

template <typename T>
void list<T>::resize(size_t n, T val) {
    if (n > size_) {
        while (size_ < n) {
            push_back(val);
        }
    } else if (n < size_) {
        while (size_ > n) {
            pop_back();
        }
    }
}

template <typename T>
void list<T>::push_front(const T& val) {
    Node* new_node = new Node(val, nullptr, head_);
    if (head_ != nullptr) {
        head_->prev = new_node;
    } else {
        tail_ = new_node;
    }
    head_ = new_node;
    size_++;
}

template <typename T>
void list<T>::pop_front() {
    if (head_ != nullptr) {
        Node* old_head = head_;
        head_ = head_->next;
        if (head_ != nullptr) {
            head_->prev = nullptr;
        } else {
            tail_ = nullptr;
        }
        delete old_head;
        size_--;
    }
}

template <typename T>
void list<T>::push_back(const T& val) {
    Node* new_node = new Node(val, tail_, nullptr);
    if (tail_ != nullptr) {
        tail_->next = new_node;
    } else {
        head_ = new_node;
    }
    tail_ = new_node;
    size_++;
}

template <typename T>
void list<T>::pop_back() {
    if (tail_ != nullptr) {
        Node* old_tail = tail_;
        tail_ = tail_->prev;
        if (tail_ != nullptr) {
            tail_->next = nullptr;
        } else {
            head_ = nullptr;
        }
        delete old_tail;
        size_--;
    }
}

template <typename T>
typename list<T>::iterator list<T>::insert(iterator position, const T& val) {
    Node* pos_node = position.node();
    Node* new_node = new Node(val, pos_node ? pos_node->prev : nullptr, pos_node);

    if (pos_node != nullptr) {
        pos_node->prev = new_node;
    } else if (tail_ == nullptr) {
        head_ = tail_ = new_node;
    }

    if (new_node->prev != nullptr) {
        new_node->prev->next = new_node;
    } else {
        head_ = new_node;
    }

    size_++;
    return iterator(new_node);
}

template <typename T>
typename list<T>::iterator list<T>::insert(iterator position, size_t n, const T& val) {
    iterator result = position;
    if (n > 0) {
        result = insert(position, val);
        for (size_t i = 1; i < n; i++) {
            insert(position, val);
        }
    }
    return result;
}

template <typename T>
typename list<T>::iterator list<T>::insert(iterator position, iterator first, iterator last) {
    iterator result = position;
    bool first_inserted = false;
    while (first != last) {
        if (!first_inserted) {
            result = insert(position, *first);
            first_inserted = true;
        } else {
            insert(position, *first);
        }
        ++first;
    }
    return result;
}

template <typename T>
typename list<T>::iterator list<T>::erase(iterator position) {
    Node* pos_node = position.node();
    if (pos_node == nullptr) {
        return position;
    }

    if (pos_node->prev != nullptr) {
        pos_node->prev->next = pos_node->next;
    } else {
        head_ = pos_node->next;
    }

    if (pos_node->next != nullptr) {
        pos_node->next->prev = pos_node->prev;
    } else {
        tail_ = pos_node->prev;
    }

    Node* next_node = pos_node->next;
    delete pos_node;
    size_--;

    return iterator(next_node);
}

template <typename T>
typename list<T>::iterator list<T>::erase(iterator first, iterator last) {
    iterator result = first;
    while (first != last) {
        first = erase(first);
    }
    return first;
}

template <typename T>
void list<T>::clear() {
    while (head_ != nullptr) {
        Node* next = head_->next;
        delete head_;
        head_ = next;
    }
    head_ = tail_ = nullptr;
    size_ = 0;
}

template <typename T>
void list<T>::splice(iterator position, list& x) {
    if (x.empty()) return;
    splice(position, x, x.begin(), x.end());
}

template <typename T>
void list<T>::splice(iterator position, list& x, iterator i) {
    iterator j = i;
    ++j;
    splice(position, x, i, j);
}

template <typename T>
void list<T>::splice(iterator position, list& x, iterator first, iterator last) {
    if (first == last) return;

    Node* pos_node = position.node();
    Node* first_node = first.node();
    Node* last_node = last.node() ? last.node()->prev : x.tail_;

    if (pos_node != nullptr) {
        pos_node->prev->next = first_node;
        first_node->prev = pos_node->prev;
    } else {
        head_ = first_node;
    }

    if (last_node != nullptr) {
        last_node->next->prev = pos_node ? pos_node->prev : nullptr;
        if (pos_node != nullptr) {
            pos_node->prev = last_node->next;
        }
    }

    first_node->prev = pos_node ? pos_node->prev : nullptr;
    if (pos_node != nullptr) {
        last_node->next = pos_node;
    } else {
        tail_ = last_node;
    }

    size_ += x.size_;
    x.head_ = x.tail_ = nullptr;
    x.size_ = 0;
}

template <typename T>
void list<T>::remove(const T& val) {
    iterator it = begin();
    while (it != end()) {
        if (*it == val) {
            it = erase(it);
        } else {
            ++it;
        }
    }
}

template <typename T>
template <class Predicate>
void list<T>::remove_if(Predicate pred) {
    iterator it = begin();
    while (it != end()) {
        if (pred(*it)) {
            it = erase(it);
        } else {
            ++it;
        }
    }
}

template <typename T>
void list<T>::unique() {
    if (size_ <= 1) return;
    iterator it = begin();
    T last_value = *it;
    ++it;
    while (it != end()) {
        if (*it == last_value) {
            it = erase(it);
        } else {
            last_value = *it;
            ++it;
        }
    }
}

template <typename T>
template <class BinaryPredicate>
void list<T>::unique(BinaryPredicate binary_pred) {
    if (size_ <= 1) return;
    iterator it = begin();
    T last_value = *it;
    ++it;
    while (it != end()) {
        if (binary_pred(*it, last_value)) {
            it = erase(it);
        } else {
            last_value = *it;
            ++it;
        }
    }
}

template <typename T>
void list<T>::merge(list& x) {
    if (this == &x) return;

    iterator this_it = begin();
    iterator x_it = x.begin();

    while (this_it != end() && x_it != x.end()) {
        if (*x_it < *this_it) {
            iterator next_x_it = x_it;
            ++next_x_it;
            splice(this_it, x, x_it, next_x_it);
            x_it = next_x_it;
        } else {
            ++this_it;
        }
    }

    if (!x.empty()) {
        splice(end(), x);
    }
}

template <typename T>
template <class Compare>
void list<T>::merge(list& x, Compare comp) {
    if (this == &x) return;

    iterator this_it = begin();
    iterator x_it = x.begin();

    while (this_it != end() && x_it != x.end()) {
        if (comp(*x_it, *this_it)) {
            iterator next_x_it = x_it;
            ++next_x_it;
            splice(this_it, x, x_it, next_x_it);
            x_it = next_x_it;
        } else {
            ++this_it;
        }
    }

    if (!x.empty()) {
        splice(end(), x);
    }
}

template <typename T>
void list<T>::sort() {
    if (size_ <= 1) return;

    list<T> result;
    list<T> temp;

    while (!empty()) {
        temp.splice(temp.begin(), *this, begin());
        result.merge(temp);
    }

    splice(result.begin(), result, result.begin(), result.end());
}

template <typename T>
template <class Compare>
void list<T>::sort(Compare comp) {
    if (size_ <= 1) return;

    list<T> result;
    list<T> temp;

    while (!empty()) {
        temp.splice(temp.begin(), *this, begin());
        result.merge(temp, comp);
    }

    splice(result.begin(), result, result.begin(), result.end());
}

template <typename T>
void list<T>::reverse() {
    if (size_ <= 1) return;

    Node* current = head_;
    Node* temp = nullptr;

    tail_ = head_;

    while (current != nullptr) {
        temp = current->prev;
        current->prev = current->next;
        current->next = temp;
        current = current->prev;
    }

    if (temp != nullptr) {
        head_ = temp->prev;
    }
}

// Explicit template instantiations
template class list<int>;

}