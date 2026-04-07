#include "iterator"

namespace std {

template <typename T>
base_iterator<T>::base_iterator() : ptr_(nullptr) {
}

template <typename T>
base_iterator<T>::base_iterator(T* ptr) : ptr_(ptr) {
}

template <typename T>
base_iterator<T>::base_iterator(const base_iterator& src) : ptr_(src.ptr_) {
}

template <typename T>
base_iterator<T>& base_iterator<T>::operator=(const base_iterator& rhs) {
    ptr_ = rhs.ptr_;
    return *this;
}

template <typename T>
base_iterator<T>& base_iterator<T>::operator++() {
    ptr_++;
    return *this;
}

template <typename T>
base_iterator<T> base_iterator<T>::operator++(int) {
    base_iterator tmp(*this);
    ptr_++;
    return tmp;
}

template <typename T>
bool base_iterator<T>::operator==(const base_iterator& rhs) const {
    return ptr_ == rhs.ptr_;
}

template <typename T>
bool base_iterator<T>::operator!=(const base_iterator& rhs) const {
    return ptr_ != rhs.ptr_;
}

template <typename T>
T& base_iterator<T>::operator*() {
    return *ptr_;
}

template <typename T>
const T& base_iterator<T>::operator*() const {
    return *ptr_;
}

template <typename T>
T* base_iterator<T>::operator->() {
    return ptr_;
}

template <typename T>
const T* base_iterator<T>::operator->() const {
    return ptr_;
}

template <typename T>
bidirectionnal_iterator<T>::bidirectionnal_iterator() : base_iterator<T>() {
}

template <typename T>
bidirectionnal_iterator<T>::bidirectionnal_iterator(T* ptr) : base_iterator<T>(ptr) {
}

template <typename T>
bidirectionnal_iterator<T>::bidirectionnal_iterator(const bidirectionnal_iterator& src) : base_iterator<T>(src.ptr_) {
}

template <typename T>
bidirectionnal_iterator<T>& bidirectionnal_iterator<T>::operator--() {
    ptr_--;
    return *this;
}

template <typename T>
bidirectionnal_iterator<T> bidirectionnal_iterator<T>::operator--(int) {
    bidirectionnal_iterator tmp(*this);
    ptr_--;
    return tmp;
}

template <typename T>
random_access_iterator<T>::random_access_iterator() : bidirectionnal_iterator<T>() {
}

template <typename T>
random_access_iterator<T>::random_access_iterator(T* ptr) : bidirectionnal_iterator<T>(ptr) {
}

template <typename T>
random_access_iterator<T>::random_access_iterator(const random_access_iterator& src) : bidirectionnal_iterator<T>(src.ptr_) {
}

template <typename T>
random_access_iterator<T> random_access_iterator<T>::operator+(size_t n) const {
    random_access_iterator tmp(*this);
    tmp.ptr_ += n;
    return tmp;
}

template <typename T>
random_access_iterator<T> random_access_iterator<T>::operator-(size_t n) const {
    random_access_iterator tmp(*this);
    tmp.ptr_ -= n;
    return tmp;
}

template <typename T>
bool random_access_iterator<T>::operator<(const random_access_iterator& rhs) const {
    return ptr_ < rhs.ptr_;
}

template <typename T>
bool random_access_iterator<T>::operator>(const random_access_iterator& rhs) const {
    return ptr_ > rhs.ptr_;
}

template <typename T>
bool random_access_iterator<T>::operator<=(const random_access_iterator& rhs) const {
    return ptr_ <= rhs.ptr_;
}

template <typename T>
bool random_access_iterator<T>::operator>=(const random_access_iterator& rhs) const {
    return ptr_ >= rhs.ptr_;
}

template <typename T>
random_access_iterator<T>& random_access_iterator<T>::operator+=(size_t n) {
    ptr_ += n;
    return *this;
}

template <typename T>
random_access_iterator<T>& random_access_iterator<T>::operator-=(size_t n) {
    ptr_ -= n;
    return *this;
}

template <typename T>
T& random_access_iterator<T>::operator[](size_t n) {
    return ptr_[n];
}

template <typename T>
const T& random_access_iterator<T>::operator[](size_t n) const {
    return ptr_[n];
}

template <typename T>
random_access_iterator<T> operator+(size_t n, const random_access_iterator<T>& rhs) {
    return rhs + n;
}

template <typename T>
random_access_iterator<T> operator-(size_t n, const random_access_iterator<T>& rhs) {
    return rhs - n;
}

}