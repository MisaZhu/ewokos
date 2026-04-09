#include "deque"
#include <stdlib.h>
#include <cstring>

namespace std {

template <typename T>
deque<T>::deque() : first_buffer_(nullptr), last_buffer_(nullptr),
                    front_index_(0), back_index_(0), size_(0) {
    first_buffer_ = new Buffer();
    last_buffer_ = first_buffer_;
    first_buffer_->data = (T*)malloc(BUFFER_SIZE * sizeof(T));
}

template <typename T>
deque<T>::deque(size_t n, const T& val) : first_buffer_(nullptr), last_buffer_(nullptr),
                                           front_index_(0), back_index_(0), size_(0) {
    first_buffer_ = new Buffer();
    last_buffer_ = first_buffer_;
    first_buffer_->data = (T*)malloc(BUFFER_SIZE * sizeof(T));
    for (size_t i = 0; i < n; i++) {
        push_back(val);
    }
}

template <typename T>
deque<T>::deque(const deque<T>& src) : first_buffer_(nullptr), last_buffer_(nullptr),
                                        front_index_(0), back_index_(0), size_(0) {
    first_buffer_ = new Buffer();
    last_buffer_ = first_buffer_;
    first_buffer_->data = (T*)malloc(BUFFER_SIZE * sizeof(T));

    Buffer* current = src.first_buffer_;
    size_t start = src.front_index_;
    size_t count = src.size_;

    while (count > 0) {
        size_t buf_start = (current == src.first_buffer_) ? start : 0;
        size_t available = BUFFER_SIZE - buf_start;
        size_t to_copy = (count < available) ? count : available;

        for (size_t i = 0; i < to_copy; i++) {
            push_back(current->data[buf_start + i]);
        }
        current = current->next;
        count -= to_copy;
    }
}

template <typename T>
deque<T>::~deque() {
    clear();
    free(first_buffer_->data);
    delete first_buffer_;
}

template <typename T>
deque<T>& deque<T>::operator=(const deque& rhs) {
    if (this != &rhs) {
        clear();
        Buffer* current = rhs.first_buffer_;
        size_t start = rhs.front_index_;
        size_t count = rhs.size_;

        while (count > 0) {
            size_t buf_start = (current == rhs.first_buffer_) ? start : 0;
            size_t available = BUFFER_SIZE - buf_start;
            size_t to_copy = (count < available) ? count : available;

            for (size_t i = 0; i < to_copy; i++) {
                push_back(current->data[buf_start + i]);
            }
            current = current->next;
            count -= to_copy;
        }
    }
    return *this;
}

template <typename T>
typename deque<T>::iterator deque<T>::begin() {
    return iterator(first_buffer_->data + front_index_);
}

template <typename T>
typename deque<T>::const_iterator deque<T>::begin() const {
    return const_iterator(first_buffer_->data + front_index_);
}

template <typename T>
typename deque<T>::iterator deque<T>::end() {
    if (back_index_ == 0 && last_buffer_ != first_buffer_) {
        return iterator(last_buffer_->prev->data + BUFFER_SIZE);
    }
    return iterator(last_buffer_->data + back_index_);
}

template <typename T>
typename deque<T>::const_iterator deque<T>::end() const {
    if (back_index_ == 0 && last_buffer_ != first_buffer_) {
        return const_iterator(last_buffer_->prev->data + BUFFER_SIZE);
    }
    return const_iterator(last_buffer_->data + back_index_);
}

template <typename T>
T& deque<T>::operator[](size_t n) {
    size_t index = front_index_ + n;
    Buffer* buf = first_buffer_;
    while (index >= BUFFER_SIZE) {
        buf = buf->next;
        index -= BUFFER_SIZE;
    }
    return buf->data[index];
}

template <typename T>
const T& deque<T>::operator[](size_t n) const {
    size_t index = front_index_ + n;
    Buffer* buf = first_buffer_;
    while (index >= BUFFER_SIZE) {
        buf = buf->next;
        index -= BUFFER_SIZE;
    }
    return buf->data[index];
}

template <typename T>
T& deque<T>::at(size_t n) {
    return (*this)[n];
}

template <typename T>
const T& deque<T>::at(size_t n) const {
    return (*this)[n];
}

template <typename T>
T& deque<T>::front() {
    return first_buffer_->data[front_index_];
}

template <typename T>
const T& deque<T>::front() const {
    return first_buffer_->data[front_index_];
}

template <typename T>
T& deque<T>::back() {
    if (back_index_ == 0) {
        return last_buffer_->prev->data[BUFFER_SIZE - 1];
    }
    return last_buffer_->data[back_index_ - 1];
}

template <typename T>
const T& deque<T>::back() const {
    if (back_index_ == 0) {
        return last_buffer_->prev->data[BUFFER_SIZE - 1];
    }
    return last_buffer_->data[back_index_ - 1];
}

template <typename T>
size_t deque<T>::size() const {
    return size_;
}

template <typename T>
bool deque<T>::empty() const {
    return size_ == 0;
}

template <typename T>
void deque<T>::resize(size_t n, T val) {
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
void deque<T>::push_front(const T& val) {
    if (front_index_ == 0) {
        Buffer* new_buf = new Buffer();
        new_buf->data = (T*)malloc(BUFFER_SIZE * sizeof(T));
        new_buf->next = first_buffer_;
        first_buffer_->prev = new_buf;
        first_buffer_ = new_buf;
        front_index_ = BUFFER_SIZE - 1;
        first_buffer_->data[front_index_] = val;
    } else {
        front_index_--;
        first_buffer_->data[front_index_] = val;
    }
    size_++;
}

template <typename T>
void deque<T>::pop_front() {
    if (front_index_ >= BUFFER_SIZE - 1) {
        Buffer* old_buf = first_buffer_;
        first_buffer_ = first_buffer_->next;
        first_buffer_->prev = nullptr;
        free(old_buf->data);
        delete old_buf;
        front_index_ = 0;
    } else {
        front_index_++;
    }
    size_--;
}

template <typename T>
void deque<T>::push_back(const T& val) {
    if (back_index_ >= BUFFER_SIZE) {
        Buffer* new_buf = new Buffer();
        new_buf->data = (T*)malloc(BUFFER_SIZE * sizeof(T));
        new_buf->prev = last_buffer_;
        last_buffer_->next = new_buf;
        last_buffer_ = new_buf;
        back_index_ = 0;
    }
    last_buffer_->data[back_index_] = val;
    back_index_++;
    size_++;
}

template <typename T>
void deque<T>::pop_back() {
    if (back_index_ == 0) {
        Buffer* old_buf = last_buffer_;
        last_buffer_ = last_buffer_->prev;
        last_buffer_->next = nullptr;
        free(old_buf->data);
        delete old_buf;
        back_index_ = BUFFER_SIZE;
    }
    back_index_--;
    size_--;
}

template <typename T>
void deque<T>::clear() {
    while (first_buffer_ != last_buffer_) {
        Buffer* next = first_buffer_->next;
        free(first_buffer_->data);
        delete first_buffer_;
        first_buffer_ = next;
    }
    front_index_ = 0;
    back_index_ = 0;
    size_ = 0;
}

// Explicit template instantiations
template class deque<int>;

}