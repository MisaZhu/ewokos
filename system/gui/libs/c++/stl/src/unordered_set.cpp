#include "unordered_set"
#include <stdlib.h>

namespace std {

template <typename T>
unordered_set<T>::unordered_set() : buckets_(nullptr), size_(0) {
    buckets_ = (unordered_set_node<T>**)calloc(BUCKET_COUNT, sizeof(unordered_set_node<T>*));
}

template <typename T>
unordered_set<T>::unordered_set(const unordered_set<T>& src) : buckets_(nullptr), size_(0) {
    buckets_ = (unordered_set_node<T>**)calloc(BUCKET_COUNT, sizeof(unordered_set_node<T>*));
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        unordered_set_node<T>* current = src.buckets_[i];
        while (current != nullptr) {
            insert(current->key);
            current = current->next;
        }
    }
}

template <typename T>
unordered_set<T>::~unordered_set() {
    clear();
    free(buckets_);
}

template <typename T>
unordered_set<T>& unordered_set<T>::operator=(const unordered_set& rhs) {
    if (this != &rhs) {
        clear();
        for (size_t i = 0; i < BUCKET_COUNT; i++) {
            unordered_set_node<T>* current = rhs.buckets_[i];
            while (current != nullptr) {
                insert(current->key);
                current = current->next;
            }
        }
    }
    return *this;
}

template <typename T>
typename unordered_set<T>::iterator unordered_set<T>::begin() {
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        if (buckets_[i] != nullptr) {
            return iterator(&buckets_[i]->key);
        }
    }
    return iterator(nullptr);
}

template <typename T>
typename unordered_set<T>::const_iterator unordered_set<T>::begin() const {
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        if (buckets_[i] != nullptr) {
            return const_iterator(&buckets_[i]->key);
        }
    }
    return const_iterator(nullptr);
}

template <typename T>
typename unordered_set<T>::iterator unordered_set<T>::end() {
    return iterator(nullptr);
}

template <typename T>
typename unordered_set<T>::const_iterator unordered_set<T>::end() const {
    return const_iterator(nullptr);
}

template <typename T>
size_t unordered_set<T>::size() const {
    return size_;
}

template <typename T>
bool unordered_set<T>::empty() const {
    return size_ == 0;
}

template <typename T>
void unordered_set<T>::clear() {
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        unordered_set_node<T>* current = buckets_[i];
        while (current != nullptr) {
            unordered_set_node<T>* next = current->next;
            delete current;
            current = next;
        }
        buckets_[i] = nullptr;
    }
    size_ = 0;
}

template <typename T>
size_t unordered_set<T>::hash(const T& val) const {
    size_t h = 0;
    const char* data = (const char*)&val;
    for (size_t i = 0; i < sizeof(T); i++) {
        h = h * 31 + data[i];
    }
    return h % BUCKET_COUNT;
}

template <typename T>
typename unordered_set<T>::iterator unordered_set<T>::find(const T& val) {
    size_t index = hash(val);
    unordered_set_node<T>* current = buckets_[index];
    while (current != nullptr) {
        if (current->key == val) {
            return iterator(&current->key);
        }
        current = current->next;
    }
    return end();
}

template <typename T>
typename unordered_set<T>::const_iterator unordered_set<T>::find(const T& val) const {
    size_t index = hash(val);
    unordered_set_node<T>* current = buckets_[index];
    while (current != nullptr) {
        if (current->key == val) {
            return const_iterator(&current->key);
        }
        current = current->next;
    }
    return end();
}

template <typename T>
size_t unordered_set<T>::insert(const T& val) {
    size_t index = hash(val);
    unordered_set_node<T>* current = buckets_[index];
    while (current != nullptr) {
        if (current->key == val) {
            return 0;
        }
        current = current->next;
    }

    unordered_set_node<T>* new_node = new unordered_set_node<T>(val);
    new_node->next = buckets_[index];
    buckets_[index] = new_node;
    size_++;
    return 1;
}

template <typename T>
size_t unordered_set<T>::erase(const T& val) {
    size_t index = hash(val);
    unordered_set_node<T>* current = buckets_[index];
    unordered_set_node<T>* prev = nullptr;

    while (current != nullptr) {
        if (current->key == val) {
            if (prev == nullptr) {
                buckets_[index] = current->next;
            } else {
                prev->next = current->next;
            }
            delete current;
            size_--;
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

template <typename T>
void unordered_set<T>::swap(unordered_set& other) {
    unordered_set_node<T>** temp_buckets = buckets_;
    buckets_ = other.buckets_;
    other.buckets_ = temp_buckets;

    size_t temp_size = size_;
    size_ = other.size_;
    other.size_ = temp_size;
}

// Explicit template instantiations
template class unordered_set<int>;

}