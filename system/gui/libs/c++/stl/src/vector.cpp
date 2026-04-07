#include "vector"
#include <stdlib.h>
#include <cstring>

namespace std {

template <typename T>
vector<T>::vector() : data_(nullptr), length_(0), capacity_(4) {
    data_ = (T*)malloc(sizeof(T) * capacity_);
    begin_ = &data_[0];
    end_ = &data_[length_];
}

template <typename T>
vector<T>::vector(size_t n, const T& val) : length_(n), capacity_(n * 2) {
    data_ = (T*)malloc(sizeof(T) * capacity_);
    for (size_t i = 0; i < length_; i++) {
        data_[i] = val;
    }

    begin_ = &data_[0];
    end_ = &data_[length_];
}

template <typename T>
vector<T>::vector(iterator first, iterator last) {
    length_ = ((size_t)&(*last) - (size_t)&(*first)) / sizeof(T);
    capacity_ = length_ * 2;
    data_ = (T*)malloc(sizeof(T) * capacity_);

    for (size_t i = 0; first != last; ++first, i++) {
        data_[i] = *first;
    }

    begin_ = &data_[0];
    end_ = &data_[length_];
}

template <typename T>
vector<T>::vector(const vector<T>& src) {
    length_ = src.length_;
    capacity_ = src.capacity_;
    data_ = (T*)malloc(sizeof(T) * capacity_);

    for (size_t i = 0; i < length_; i++) {
        data_[i] = src.data_[i];
    }

    begin_ = &data_[0];
    end_ = &data_[length_];
}

template <typename T>
vector<T>::~vector() {
    clear();
    free(data_);
}

template <typename T>
vector<T>& vector<T>::operator=(const vector& rhs) {
    if (this != &rhs) {
        clear();
        free(data_);

        length_ = rhs.length_;
        capacity_ = rhs.capacity_;
        data_ = (T*)malloc(sizeof(T) * capacity_);

        for (size_t i = 0; i < length_; i++) {
            data_[i] = rhs.data_[i];
        }

        begin_ = &data_[0];
        end_ = &data_[length_];
    }

    return *this;
}

template <typename T>
typename vector<T>::iterator vector<T>::begin() {
    return begin_;
}

template <typename T>
const typename vector<T>::const_iterator vector<T>::begin() const {
    return begin_;
}

template <typename T>
typename vector<T>::iterator vector<T>::end() {
    return end_;
}

template <typename T>
const typename vector<T>::const_iterator vector<T>::end() const {
    return end_;
}

template <typename T>
T& vector<T>::front() {
    return data_[0];
}

template <typename T>
const T& vector<T>::front() const {
    return data_[0];
}

template <typename T>
T& vector<T>::back() {
    return data_[length_ - 1];
}

template <typename T>
const T& vector<T>::back() const {
    return data_[length_ - 1];
}

template <typename T>
void vector<T>::resize(size_t n, T val) {
    if (n < length_) {
        capacity_ = n * 2;

        T* newData = (T*)malloc(sizeof(T) * capacity_);
        for (size_t i = 0; i < n; i++) {
            newData[i] = data_[i];
        }

        for (size_t i = n; i < length_; i++) {
            data_[i].~T();
        }

        free(data_);
        data_ = newData;
    } else if (n > length_) {
        if (n > capacity_) {
            capacity_ = n * 2;

            T* newData = (T*)malloc(sizeof(T) * capacity_);
            for (size_t i = 0; i < length_; i++) {
                newData[i] = data_[i];
            }

            free(data_);
            data_ = newData;
        }

        for (size_t i = length_; i < n; i++) {
            data_[i] = val;
        }
    }

    length_ = n;
    begin_ = &data_[0];
    end_ = &data_[length_];
}

template <typename T>
void vector<T>::reserve(size_t n) {
    if (n > capacity_) {
        capacity_ = n;

        T* newData = (T*)malloc(sizeof(T) * capacity_);
        for (size_t i = 0; i < length_; i++) {
            newData[i] = data_[i];
        }

        free(data_);
        data_ = newData;

        begin_ = &data_[0];
        end_ = &data_[length_];
    }
}

template <typename T>
T& vector<T>::operator[](size_t n) {
    return data_[n];
}

template <typename T>
const T& vector<T>::operator[](size_t n) const {
    return data_[n];
}

template <typename T>
T& vector<T>::at(size_t n) {
    return data_[n];
}

template <typename T>
const T& vector<T>::at(size_t n) const {
    return data_[n];
}

template <typename T>
void vector<T>::assign(iterator first, iterator last) {
    clear();

    length_ = ((size_t)&(*last) - (size_t)&(*first)) / sizeof(T);
    reserve(length_ * 2);

    for (size_t i = 0; first != last; ++first, i++) {
        data_[i] = *first;
    }

    begin_ = &data_[0];
    end_ = &data_[length_];
}

template <typename T>
void vector<T>::assign(size_t n, const T& val) {
    clear();
    reserve(n * 2);

    for (size_t i = 0; i < n; i++) {
        data_[i] = val;
    }

    length_ = n;
    end_ = &data_[length_];
}

template <typename T>
void vector<T>::push_back(const T& val) {
    if (length_ + 1 >= capacity_) {
        capacity_ *= 2;

        T* newData = (T*)malloc(sizeof(T) * capacity_);
        for (size_t i = 0; i < length_; i++) {
            newData[i] = data_[i];
        }

        free(data_);
        data_ = newData;
        begin_ = &data_[0];
    }

    data_[length_++] = val;
    end_ = &data_[length_];
}

template <typename T>
void vector<T>::pop_back() {
    data_[length_-1].~T();
    length_ -= 1;
    end_ = &data_[length_];
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(iterator position, const T& val) {
    size_t pos = ((size_t)&(*position) - (size_t)&(*begin_)) / sizeof(T);

    if (length_ + 1 > capacity_) {
        capacity_ *= 2;
        T* newData = (T*)malloc(capacity_ * sizeof(T));

        for (size_t i = 0; i < length_; i++) {
            newData[i] = data_[i];
        }

        free(data_);
        data_ = newData;

        begin_ = &data_[0];
    }

    T nextVal = val;
    T curVal;
    for (size_t i = pos; i < length_; i++) {
        curVal = data_[i];
        data_[i] = nextVal;
        nextVal = curVal;
    }

    data_[length_] = nextVal;
    length_ += 1;

    end_ = &data_[length_];

    return begin_ + pos;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(iterator position, size_t n, const T& val) {
    size_t pos = ((size_t)&(*position) - (size_t)&(*begin_)) / sizeof(T);

    if (length_ + n > capacity_) {
        capacity_ += n;
        T* newData = (T*)malloc(capacity_ * sizeof(T));

        for (size_t i = 0; i < length_; i++) {
            newData[i] = data_[i];
        }

        free(data_);
        data_ = newData;

        begin_ = &data_[0];
    }

    T* rightData = (T*)malloc((length_ - pos) * sizeof(T));
    for (size_t i = pos, idx = 0; i < length_; i++, idx++) {
        rightData[idx] = data_[i];
    }

    for (size_t i = pos; i < (pos + n); i++) {
        data_[i] = val;
    }

    for (size_t i = (pos + n), idx = 0; i < (n + length_); i++, idx++) {
        data_[i] = rightData[idx];
    }

    free(rightData);
    length_ += n;

    end_ = &data_[length_];

    return begin_ + pos;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(iterator position, iterator first, iterator last) {
    size_t pos = ((size_t)&(*position) - (size_t)&(*begin_)) / sizeof(T);
    size_t size = ((size_t)&(*last) - (size_t)&(*first)) / sizeof(T);

    if (length_ + size > capacity_) {
        capacity_ += size;
        T* newData = (T*)malloc(capacity_ * sizeof(T));

        for (size_t i = 0; i < length_; i++) {
            newData[i] = data_[i];
        }

        free(data_);
        data_ = newData;

        begin_ = &data_[0];
    }

    T* rightData = (T*)malloc((length_ - pos) * sizeof(T));
    for (size_t i = pos, idx = 0; i < length_; i++, idx++) {
        rightData[idx] = data_[i];
    }

    for (size_t i = pos; i < (pos + size); i++, ++first) {
        data_[i] = *first;
    }

    for (size_t i = (pos + size), idx = 0; i < (size + length_); i++, idx++) {
        data_[i] = rightData[idx];
    }

    free(rightData);
    length_ += size;

    end_ = &data_[length_];

    return begin_ + pos;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(iterator position) {
    size_t pos = ((size_t)&(*position) - (size_t)&(*begin_)) / sizeof(T);
    data_[length_ - 1].~T();
    length_ -= 1;

    for (size_t i = pos; i < length_; i++) {
        data_[i] = data_[i + 1];
    }

    end_ = &data_[length_];

    return begin_ + pos;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(iterator first, iterator last) {
    size_t pos = ((size_t)&(*first) - (size_t)&(*begin_)) / sizeof(T);
    size_t endPos = ((size_t)&(*last) - (size_t)&(*begin_)) / sizeof(T);

    for (size_t i = pos; i < endPos; i++) {
        data_[i].~T();
    }

    size_t diff = endPos - pos;
    length_ -= diff;
    for (size_t i = pos; i < length_; i++) {
        data_[i] = data_[i + diff];
    }

    end_ = &data_[length_];

    return begin_ + pos;
}

template <typename T>
void vector<T>::clear() {
    for (size_t i = 0; i < length_; i++) {
        data_[i].~T();
    }
    end_ = begin_;
    length_ = 0;
}

// Explicit template instantiations
template class vector<int>;

}