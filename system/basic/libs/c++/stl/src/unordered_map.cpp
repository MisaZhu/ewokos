#include "unordered_map"
#include <stdlib.h>

namespace std {

template <typename Key, typename T>
unordered_map<Key, T>::unordered_map() : buckets_(nullptr), size_(0) {
    buckets_ = (unordered_map_node<Key, T>**)calloc(BUCKET_COUNT, sizeof(unordered_map_node<Key, T>*));
}

template <typename Key, typename T>
unordered_map<Key, T>::unordered_map(const unordered_map<Key, T>& src) : buckets_(nullptr), size_(0) {
    buckets_ = (unordered_map_node<Key, T>**)calloc(BUCKET_COUNT, sizeof(unordered_map_node<Key, T>*));
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        unordered_map_node<Key, T>* current = src.buckets_[i];
        while (current != nullptr) {
            insert(current->key, current->value);
            current = current->next;
        }
    }
}

template <typename Key, typename T>
unordered_map<Key, T>::~unordered_map() {
    clear();
    free(buckets_);
}

template <typename Key, typename T>
unordered_map<Key, T>& unordered_map<Key, T>::operator=(const unordered_map& rhs) {
    if (this != &rhs) {
        clear();
        for (size_t i = 0; i < BUCKET_COUNT; i++) {
            unordered_map_node<Key, T>* current = rhs.buckets_[i];
            while (current != nullptr) {
                insert(current->key, current->value);
                current = current->next;
            }
        }
    }
    return *this;
}

template <typename Key, typename T>
typename unordered_map<Key, T>::iterator unordered_map<Key, T>::begin() {
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        if (buckets_[i] != nullptr) {
            return iterator(&buckets_[i]->value);
        }
    }
    return iterator(nullptr);
}

template <typename Key, typename T>
typename unordered_map<Key, T>::const_iterator unordered_map<Key, T>::begin() const {
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        if (buckets_[i] != nullptr) {
            return const_iterator(&buckets_[i]->value);
        }
    }
    return const_iterator(nullptr);
}

template <typename Key, typename T>
typename unordered_map<Key, T>::iterator unordered_map<Key, T>::end() {
    return iterator(nullptr);
}

template <typename Key, typename T>
typename unordered_map<Key, T>::const_iterator unordered_map<Key, T>::end() const {
    return const_iterator(nullptr);
}

template <typename Key, typename T>
size_t unordered_map<Key, T>::size() const {
    return size_;
}

template <typename Key, typename T>
bool unordered_map<Key, T>::empty() const {
    return size_ == 0;
}

template <typename Key, typename T>
void unordered_map<Key, T>::clear() {
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        unordered_map_node<Key, T>* current = buckets_[i];
        while (current != nullptr) {
            unordered_map_node<Key, T>* next = current->next;
            delete current;
            current = next;
        }
        buckets_[i] = nullptr;
    }
    size_ = 0;
}

template <typename Key, typename T>
size_t unordered_map<Key, T>::hash(const Key& key) const {
    size_t h = 0;
    const char* data = (const char*)&key;
    for (size_t i = 0; i < sizeof(Key); i++) {
        h = h * 31 + data[i];
    }
    return h % BUCKET_COUNT;
}

template <typename Key, typename T>
typename unordered_map<Key, T>::iterator unordered_map<Key, T>::find(const Key& key) {
    size_t index = hash(key);
    unordered_map_node<Key, T>* current = buckets_[index];
    while (current != nullptr) {
        if (current->key == key) {
            return iterator(&current->value);
        }
        current = current->next;
    }
    return end();
}

template <typename Key, typename T>
typename unordered_map<Key, T>::const_iterator unordered_map<Key, T>::find(const Key& key) const {
    size_t index = hash(key);
    unordered_map_node<Key, T>* current = buckets_[index];
    while (current != nullptr) {
        if (current->key == key) {
            return const_iterator(&current->value);
        }
        current = current->next;
    }
    return end();
}

template <typename Key, typename T>
size_t unordered_map<Key, T>::insert(const Key& key, const T& value) {
    size_t index = hash(key);
    unordered_map_node<Key, T>* current = buckets_[index];
    while (current != nullptr) {
        if (current->key == key) {
            return 0;
        }
        current = current->next;
    }

    unordered_map_node<Key, T>* new_node = new unordered_map_node<Key, T>(key, value);
    new_node->next = buckets_[index];
    buckets_[index] = new_node;
    size_++;
    return 1;
}

template <typename Key, typename T>
T& unordered_map<Key, T>::operator[](const Key& key) {
    size_t index = hash(key);
    unordered_map_node<Key, T>* current = buckets_[index];
    while (current != nullptr) {
        if (current->key == key) {
            return current->value;
        }
        current = current->next;
    }

    insert(key, T());
    iterator it = find(key);
    return *it;
}

template <typename Key, typename T>
size_t unordered_map<Key, T>::erase(const Key& key) {
    size_t index = hash(key);
    unordered_map_node<Key, T>* current = buckets_[index];
    unordered_map_node<Key, T>* prev = nullptr;

    while (current != nullptr) {
        if (current->key == key) {
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

template <typename Key, typename T>
void unordered_map<Key, T>::swap(unordered_map& other) {
    unordered_map_node<Key, T>** temp_buckets = buckets_;
    buckets_ = other.buckets_;
    other.buckets_ = temp_buckets;

    size_t temp_size = size_;
    size_ = other.size_;
    other.size_ = temp_size;
}

// Explicit template instantiations
template class unordered_map<int, int>;

}