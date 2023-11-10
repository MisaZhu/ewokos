// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include <cstring>
#include <memory>
#include <new>
#include <ostream>
#include <typeinfo>

#include "algorithm.hpp"
#include "allocators.hpp"
#include "iterator.hpp"

namespace tiny_stl {

namespace {

template <typename T, typename... Args>
inline void constructInPlace(T& dst, Args&&... args) noexcept(
    (is_nothrow_constructible<T, Args...>::value)) {
    ::new (static_cast<void*>(&dst)) T(tiny_stl::forward<Args>(args)...);
}

template <typename T>
inline void destroyInPlace(T& obj) noexcept {
    obj.~T();
}

// use plain function to copy/fill

template <typename InIt, typename FwdIt>
inline FwdIt uninitializedCopyAux(InIt first, InIt last, FwdIt dst,
                                  false_type /* no special optimization */) {
    for (; first != last; ++dst, ++first)
        constructInPlace(*dst, *first);

    return dst;
}

template <typename InIt, typename FwdIt>
inline FwdIt uninitializedCopyAux(InIt first, InIt last, FwdIt dst,
                                  true_type /* is pod -- assign */) {
    for (; first != last; ++first, ++dst)
        *dst = *first;

    return dst;
}

template <typename InIt, typename Size, typename FwdIt>
inline FwdIt uninitializedCopyNAux(InIt first, Size n, FwdIt dst,
                                   false_type /* no special optimization */) {
    for (; n > 0; ++dst, ++first, --n)
        constructInPlace(*dst, *first);

    return dst;
}

template <typename InIt, typename Size, typename FwdIt>
inline FwdIt uninitializedCopyNAux(InIt first, Size n, FwdIt dst,
                                   true_type /* is pod -- assign */) {
    for (; n > 0; ++first, ++dst, --n)
        *dst = *first;

    return dst;
}

template <typename FwdIt, typename T>
inline void uninitializedFillAux(FwdIt first, FwdIt last, const T& x,
                                 false_type /* is not pod type */) {

    for (; first != last; ++first)
        constructInPlace(*first, x);
}

template <typename FwdIt, typename T>
inline void uninitializedFillAux(FwdIt first, FwdIt last, const T& x,
                                 true_type /* is pod -- assign */) {
    for (; first != last; ++first)
        *first = x;
}

template <typename FwdIt, typename Size, typename T>
inline void uninitializedFillNAux(FwdIt first, Size n, const T& x,
                                  false_type /* is not pod type */) {

    for (; n--; ++first)
        constructInPlace(*first, x);
}

template <typename FwdIt, typename Size, typename T>
inline void uninitializedFillNAux(FwdIt first, Size n, const T& x,
                                  true_type /* is pod -- copy */) {
    for (; n--; ++first)
        *first = x;
}

} // namespace

template <typename InIter, typename FwdIter>
inline FwdIter uninitialized_copy(InIter first, InIter last, FwdIter dst) {
    using T = typename iterator_traits<InIter>::value_type;
    return uninitializedCopyAux(first, last, dst,
                                bool_constant<is_pod<T>::value>{});
}

// const char* and const wchar_t* version
inline char* uninitialized_copy(const char* first, const char* last,
                                char* dst) {
    std::memmove(dst, first, sizeof(char) * (last - first));
    return dst + (last - first);
}

inline wchar_t* uninitialized_copy(const wchar_t* first, const wchar_t* last,
                                   wchar_t* dst) {
    std::memmove(dst, first, sizeof(wchar_t) * (last - first));
    return dst + (last - first);
}

template <typename InIter, typename Size, typename FwdIter>
inline FwdIter uninitialized_copy_n(InIter first, Size n, FwdIter dst) {
    using T = typename iterator_traits<InIter>::value_type;
    return uninitializedCopyNAux(first, n, dst,
                                 integral_constant<bool, is_pod_v<T>>{});
}

inline char* uninitialized_copy_n(const char* first, std::size_t n, char* dst) {
    std::memmove(dst, first, n);
    return dst + n;
}

inline wchar_t* uninitialized_copy_n(const wchar_t* first, std::size_t n,
                                     wchar_t* dst) {
    std::memmove(dst, first, n * sizeof(wchar_t));
    return dst + n;
}

// use x to construct [first, last)
template <typename FwdIter, typename T>
inline void uninitialized_fill(FwdIter first, FwdIter last, const T& x) {
    uninitializedFillAux(first, last, x,
                         integral_constant<bool, is_pod_v<T>>{});
}

// use x to construct [first, first + n)
template <typename FwdIter, typename Size, typename T>
inline void uninitialized_fill_n(FwdIter first, Size n, const T& x) {
    uninitializedFillNAux(first, n, x, integral_constant<bool, is_pod_v<T>>{});
}

// use allocator to copy/fill

template <typename Alloc, typename = void>
struct IsDefaultAllocator : false_type {};

template <typename T>
struct IsDefaultAllocator<allocator<T>,
                          typename allocator<T>::NotUserSpecialized>
    : true_type {};

template <typename Void, typename... Ts>
struct HasNoAllocConstruct : true_type {};

template <typename Alloc, typename Ptr, typename... Args>
struct HasNoAllocConstruct<
    void_t<decltype(tiny_stl::declval<Alloc&>().construct(
        tiny_stl::declval<Ptr>(), tiny_stl::declval<Args>()...))>,
    Alloc, Ptr, Args...> : false_type {};

template <typename Alloc, typename Ptr, typename... Args>
using UseDefaultConstruct =
    disjunction<IsDefaultAllocator<Alloc>,
                HasNoAllocConstruct<void, Alloc, Ptr, Args...>>;

template <typename Alloc, typename Ptr, typename = void>
struct HasNoAllocDestroy : true_type {};

template <typename Alloc, typename Ptr>
struct HasNoAllocDestroy<Alloc, Ptr,
                         void_t<decltype(tiny_stl::declval<Alloc&>().destroy(
                             tiny_stl::declval<Ptr>()))>> : false_type {};

template <typename Alloc, typename Ptr>
using UseDefaultDestroy =
    disjunction<IsDefaultAllocator<Alloc>, HasNoAllocDestroy<Alloc, Ptr>>;

template <typename FwdIter, typename Alloc>
inline void destroyAllocRangeAux(FwdIter first, FwdIter last, Alloc&,
                                 true_type) {
    // do nothing
}

template <typename FwdIter, typename Alloc>
inline void destroyAllocRangeAux(FwdIter first, FwdIter last, Alloc& alloc,
                                 false_type) {
    for (; first != last; ++first)
        alloc.destroy(tiny_stl::addressof(*first));
}

template <typename FwdIter, typename Alloc>
inline void destroyAllocRange(FwdIter first, FwdIter last, Alloc& alloc) {
    using T = typename Alloc::value_type;
    destroyAllocRangeAux(
        first, last, alloc,
        typename conjunction<is_trivially_destructible<T>,
                             UseDefaultDestroy<Alloc, T*>>::type{});
}

template <typename FwdIter>
inline void destroyRangeAux(FwdIter, FwdIter, true_type) {
    // do nothing
}

template <typename FwdIter>
inline void destroyRangeAux(FwdIter first, FwdIter last, false_type) {
    for (; first != last; ++first)
        destroyInPlace(*first);
}

template <typename FwdIter>
inline void destroyRange(FwdIter first, FwdIter last) {
    destroyRangeAux(
        first, last,
        is_trivially_destructible<details::IteratorValueType<FwdIter>>{});
}

template <typename FwdIter>
inline void destroy(FwdIter first, FwdIter last) {
    destroyRange(first, last);
}

template <typename FwdIter, typename Size, typename Alloc>
inline FwdIter
uninitAllocFillNAux(FwdIter first, Size n,
                    const typename iterator_traits<FwdIter>::value_type& val,
                    Alloc& alloc, false_type /* fill memset is not safe */) {

    for (; n > 0; --n, ++first)
        alloc.construct(tiny_stl::addressof(*first), val);

    return first;
}

template <typename FwdIter, typename Size, typename Alloc>
inline FwdIter
uninitAllocFillNAux(FwdIter first, Size n,
                    const typename iterator_traits<FwdIter>::value_type& val,
                    Alloc&, true_type /* fill memset is safe */) {
    ::memset(first, val, n);
    return first + n;
}

template <typename FwdIter, typename Size, typename Alloc>
inline FwdIter uninitializedAllocFillN(
    FwdIter first, Size n,
    const typename iterator_traits<FwdIter>::value_type& val, Alloc& alloc) {
    // if memset safe && (default allocator || not user allocator)
    // then use memset
    return uninitAllocFillNAux(
        first, n, val, alloc,
        typename conjunction<
            decltype(tiny_stl::details::fillMemsetIsSafe(first, val)),
            UseDefaultConstruct<Alloc, decltype(tiny_stl::addressof(*first)),
                                decltype(val)>>::type{});
}

template <typename FwdIter>
using UseMemsetValueConstructType = typename conjunction<
    is_pointer<FwdIter>, is_scalar<details::IteratorValueType<FwdIter>>,
    negation<is_volatile<details::IteratorValueType<FwdIter>>>,
    negation<is_member_pointer<details::IteratorValueType<FwdIter>>>>::type;

template <typename FwdIter>
inline FwdIter zeroMemsetRangeAux(FwdIter first, FwdIter last) {
    char* const first_ch = reinterpret_cast<char*>(first);
    char* const last_ch = reinterpret_cast<char*>(last);
    ::memset(first_ch, 0, last_ch - first_ch);
    return last;
}

template <typename FwdIter, typename Size, typename Alloc>
inline FwdIter uninitAllocDefaultNAux(FwdIter first, Size n, Alloc&,
                                      true_type) {
    return zeroMemsetRangeAux(first, first + n);
}

template <typename FwdIter, typename Size, typename Alloc>
inline FwdIter uninitAllocDefaultNAux(FwdIter first, Size n, Alloc& alloc,
                                      false_type) {
    alloc.construct(tiny_stl::addressof(*first));
    return first;
}

template <typename FwdIter, typename Size, typename Alloc>
inline FwdIter uninitializedAllocDefaultN(FwdIter first, Size n, Alloc& alloc) {
    return uninitAllocDefaultNAux(
        first, n, alloc,
        typename conjunction<
            UseMemsetValueConstructType<FwdIter>,
            UseDefaultConstruct<Alloc, decltype(tiny_stl::addressof(*first))>>::
            type{});
}

template <typename InIter, typename FwdIter, typename Alloc>
inline FwdIter uninitializedAllocCopyAux(InIter first, InIter last,
                                         FwdIter newFirst, Alloc&,
                                         true_type /*pod*/) {
    for (; first != last; ++first, ++newFirst)
        *newFirst = *first;

    return newFirst;
}

template <typename InIter, typename FwdIter, typename Alloc>
inline FwdIter uninitializedAllocCopyAux(InIter first, InIter last,
                                         FwdIter newFirst, Alloc& alloc,
                                         false_type) {

    for (; first != last; ++first, ++newFirst)
        alloc.construct(newFirst, *first);

    return newFirst;
}

template <typename InIter, typename FwdIter, typename Alloc>
inline FwdIter uninitializedAllocCopy(InIter first, InIter last,
                                      FwdIter newFirst, Alloc& alloc) {
    using T = typename iterator_traits<InIter>::value_type;
    return uninitializedAllocCopyAux(first, last, newFirst, alloc,
                                     bool_constant<is_pod<T>::value>{});
}

template <typename InIter, typename FwdIter, typename Alloc>
inline FwdIter uninitializedAllocMove(InIter first, InIter last,
                                      FwdIter newFirst, Alloc& alloc) {

    for (; first != last; ++newFirst, ++first) {
        allocator_traits<Alloc>::construct(
            alloc, tiny_stl::addressof(*newFirst), tiny_stl::move(*first));
    }

    return newFirst;
}

template <typename T>
struct GetFirstParameter;

template <template <typename, typename...> class T, typename First,
          typename... Rest>
struct GetFirstParameter<T<First, Rest...>> {
    using type = First;
};

template <typename T, typename = void>
struct GetElementType {
    using type = typename GetFirstParameter<T>::type;
};

template <typename T>
struct GetElementType<T, void_t<typename T::element_type>> {
    using type = typename T::element_type;
};

template <typename T, typename = void>
struct GetPtrDifferenceType {
    using type = std::ptrdiff_t;
};

template <typename T>
struct GetPtrDifferenceType<T, void_t<typename T::difference_type>> {
    using type = typename T::difference_type;
};

template <typename NewFirst, typename T>
struct ReplaceFirstParameter;

template <typename NewFirst, template <typename, typename...> class T,
          typename First, typename... Rest>
struct ReplaceFirstParameter<NewFirst, T<First, Rest...>> {
    using type = T<NewFirst, Rest...>;
};

template <typename T, typename Other, typename = void>
struct GetRebindType {
    using type = typename ReplaceFirstParameter<Other, T>::type;
};

template <typename T, typename Other>
struct GetRebindType<T, Other, void_t<typename T::template rebind<Other>>> {
    using type = typename T::template rebind<Other>;
};

// deal with type of like pointer
template <typename Ptr>
struct pointer_traits {
    using pointer = Ptr;
    using element_type = typename GetElementType<Ptr>::type;
    using difference_type = typename GetPtrDifferenceType<Ptr>::type;

    template <typename Other>
    using rebind = typename GetRebindType<Ptr, Other>::type;

    using RefType = conditional_t<is_void<element_type>::value, char&,
                                  add_lvalue_reference_t<element_type>>;

    static pointer pointer_to(RefType val) {
        return Ptr::pointer_to(val);
    }
};

template <typename T>
struct pointer_traits<T*> {
    using element_type = T;
    using pointer = T*;
    using difference_type = std::ptrdiff_t;

    template <typename Other>
    using rebind = Other*;

    using RefType =
        conditional_t<is_void<T>::value, char&, add_lvalue_reference_t<T>>;

    static pointer pointer_to(RefType val) {
        return tiny_stl::addressof(val);
    }
};

// allocator_traits type helper
template <typename Alloc, typename = void>
struct GetPointer {
    using type = typename Alloc::value_type*;
};

template <typename Alloc>
struct GetPointer<Alloc, void_t<typename Alloc::pointer>> {
    using type = typename Alloc::pointer;
};

template <typename Alloc, typename = void>
struct GetConstPointer {
    using Ptr = typename GetPointer<Alloc>::type;
    using Val = typename Alloc::value_type;
    using type = typename pointer_traits<Ptr>::template rebind<const Val>;
};

template <typename Alloc>
struct GetConstPointer<Alloc, void_t<typename Alloc::const_pointer>> {
    using type = typename Alloc::const_pointer;
};

template <typename Alloc, typename = void>
struct GetVoidPointer {
    using Ptr = typename GetPointer<Alloc>::type;
    using type = typename pointer_traits<Ptr>::template rebind<void>;
};

template <typename Alloc>
struct GetVoidPointer<Alloc, void_t<typename Alloc::void_pointer>> {
    using type = typename Alloc::void_pointer;
};

template <typename Alloc, typename = void>
struct GetConstVoidPointer {
    using Ptr = typename GetPointer<Alloc>::type;
    using type = typename pointer_traits<Ptr>::template rebind<const void>;
};

template <typename Alloc>
struct GetConstVoidPointer<Alloc, void_t<typename Alloc::const_void_pointer>> {
    using type = typename Alloc::const_void_pointer;
};

template <typename Alloc, typename = void>
struct GetDifferenceType {
    using Ptr = typename GetPointer<Alloc>::type;
    using type = typename pointer_traits<Ptr>::difference_type;
};

template <typename Alloc>
struct GetDifferenceType<Alloc, void_t<typename Alloc::difference_type>> {
    using type = typename Alloc::difference_type;
};

template <typename Alloc, typename = void>
struct GetSizeType {
    using Diff = typename GetDifferenceType<Alloc>::type;
    using type = make_unsigned_t<Diff>;
};

template <typename Alloc>
struct GetSizeType<Alloc, void_t<typename Alloc::size_type>> {
    using type = typename Alloc::size_type;
};

template <typename Alloc, typename = void>
struct GetPropagateOnContainerCopy {
    using type = false_type;
};

template <typename Alloc>
struct GetPropagateOnContainerCopy<
    Alloc, void_t<typename Alloc::propagate_on_container_copy_assignment>> {
    using type = typename Alloc::propagate_on_container_copy_assignment;
};

template <typename Alloc, typename = void>
struct GetPropagateOnContainerMove {
    using type = false_type;
};

template <typename Alloc>
struct GetPropagateOnContainerMove<
    Alloc, void_t<typename Alloc::propagate_on_container_move_assignment>> {
    using type = typename Alloc::propagate_on_container_move_assignment;
};

template <class Alloc, typename = void>
struct GetPropagateOnContainerSwap {
    using type = false_type;
};

template <class Alloc>
struct GetPropagateOnContainerSwap<
    Alloc, void_t<typename Alloc::propagate_on_container_swap>> {
    using type = typename Alloc::propagate_on_container_swap;
};

template <class Alloc, typename = void>
struct GetIsAlwaysEqual {
    using type = typename is_empty<Alloc>::type;
};

template <class Alloc>
struct GetIsAlwaysEqual<Alloc, void_t<typename Alloc::is_always_equal>> {
    using type = typename Alloc::is_always_equal;
};

template <typename Alloc, typename Other, typename = void>
struct GetAllocBindType {
    using type = typename ReplaceFirstParameter<Other, Alloc>::type;
};

template <typename Alloc, typename Other>
struct GetAllocBindType<Alloc, Other,
                        void_t<typename Alloc::template rebind<Other>::other>> {
    using type = typename Alloc::template rebind<Other>::other;
};

template <typename Alloc, typename Size_type, typename Const_void_pointer,
          typename = void>
struct HasAllocateHint : false_type {};

template <typename Alloc, typename Size_type, typename Const_void_pointer>
struct HasAllocateHint<Alloc, Size_type, Const_void_pointer,
                       void_t<decltype(tiny_stl::declval<Alloc&>().allocate(
                           tiny_stl::declval<const Size_type&>(),
                           tiny_stl::declval<const Const_void_pointer&>()))>>
    : true_type {};

template <typename Alloc, typename = void>
struct HasMaxSize : false_type {};

template <typename Alloc>
struct HasMaxSize<
    Alloc, void_t<decltype(tiny_stl::declval<const Alloc&>().max_size())>>
    : true_type {};

template <typename Alloc, typename = void>
struct HasSelectOnContainerCopyConstruction : false_type {};

template <typename Alloc>
struct HasSelectOnContainerCopyConstruction<
    Alloc, void_t<decltype(tiny_stl::declval<const Alloc&>()
                               .select_on_container_copy_construction())>>
    : true_type {};

template <typename Alloc>
struct allocator_traits {
    using allocator_type = Alloc;
    using value_type = typename Alloc::value_type;
    using pointer = typename GetPointer<Alloc>::type;
    using const_pointer = typename GetConstPointer<Alloc>::type;
    using void_pointer = typename GetVoidPointer<Alloc>::type;
    using const_void_pointer = typename GetConstVoidPointer<Alloc>::type;
    using size_type = typename GetSizeType<Alloc>::type;
    using difference_type = typename GetDifferenceType<Alloc>::type;

    using propagate_on_container_copy_assignment =
        typename GetPropagateOnContainerCopy<Alloc>::type;
    using propagate_on_container_move_assignment =
        typename GetPropagateOnContainerMove<Alloc>::type;
    using propagate_on_container_swap =
        typename GetPropagateOnContainerSwap<Alloc>::type;
    using is_always_equal = typename GetIsAlwaysEqual<Alloc>::type;

    template <typename Other>
    using rebind_alloc = typename GetAllocBindType<Alloc, Other>::type;

    template <typename Other>
    using rebind_traits = allocator_traits<rebind_alloc<Other>>;

    static pointer allocate(Alloc& a, const size_type n) {
        return a.allocate(n);
    }

    static pointer allocateAux(Alloc& a, const size_type n,
                               const_void_pointer hint, true_type) {
        return a.allocate(n, hint);
    }

    static pointer allocateAux(Alloc& a, const size_type n,
                               const_void_pointer hint, false_type) {
        return a.allocate(n);
    }

    static pointer allocate(Alloc& a, const size_type n,
                            const_void_pointer hint) {
        return allocateAux(
            a, n, hint,
            HasAllocateHint<Alloc, size_type, const_void_pointer>{});
    }

    static void deallocate(Alloc& a, pointer ptr, size_type n) {
        a.deallocate(ptr, n);
    }

    template <typename T, typename... Args>
    static void constructHelper(true_type, Alloc&, T* ptr, Args&&... args) {
        new (static_cast<void*>(ptr)) T(tiny_stl::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    static void constructHelper(false_type, Alloc& a, T* ptr, Args&&... args) {
        a.construct(ptr, tiny_stl::forward<Args>(args)...);
    }

    template <typename Obj, typename... Args>
    static void construct(Alloc& a, Obj* ptr, Args&&... args) {
        constructHelper(UseDefaultConstruct<Alloc, Obj*, Args...>{}, a, ptr,
                        tiny_stl::forward<Args>(args)...);
    }

    template <typename T>
    static void destroyAux(Alloc& a, T* ptr, true_type) {
        ptr->~T();
    }

    template <typename T>
    static void destroyAux(Alloc& a, T* ptr, false_type) {
        a.destroy(ptr);
    }

    template <typename T>
    static void destroy(Alloc& a, T* ptr) {
        destroyAux(a, ptr, UseDefaultDestroy<Alloc, T*>{});
    }

    static size_type maxSizeAux(const Alloc& a, true_type) {
        return a.max_size();
    }

    static size_type maxSizeAux(const Alloc& a, false_type) {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

    static size_type max_size(const Alloc& a) {
        return maxSizeAux(a, HasMaxSize<Alloc>{});
    }

    static Alloc selectOnContainerCopyConstructionAux(const Alloc& a,
                                                      true_type) {
        return a.select_on_container_copy_construction();
    }

    static Alloc selectOnContainerCopyConstructionAux(const Alloc& a,
                                                      false_type) {
        return a;
    }

    static Alloc select_on_container_copy_construction(const Alloc& a) {
        return selectOnContainerCopyConstructionAux(
            a, HasSelectOnContainerCopyConstruction<Alloc>{});
    }
}; // class allocator_traits<Alloc>

template <typename Con, typename Alloc, typename = void>
struct HasAllocatorType : false_type {};

template <typename Con, typename Alloc>
struct HasAllocatorType<Con, Alloc, void_t<typename Con::allocator_type>>
    : is_convertible<Alloc, typename Con::allocator_type>::type {};

template <typename Alloc, typename ValueType>
using RebindAllocType =
    typename allocator_traits<Alloc>::template rebind_alloc<ValueType>;

template <typename Con, typename Alloc>
struct uses_allocator : HasAllocatorType<Con, Alloc>::type {};

template <typename Con, typename Alloc>
constexpr bool uses_allocator_value = uses_allocator<Con, Alloc>::value;

template <typename T>
struct default_delete {
    constexpr default_delete() noexcept = default;

    template <typename U, typename = enable_if_t<is_convertible<U*, T*>::value>>
    default_delete(const default_delete<U>&) noexcept {
    }

    void operator()(T* ptr) const {
        static_assert(sizeof(T) > 0, "delete a incomplete type, ub");
        delete ptr;
    }
};

template <typename T>
class default_delete<T[]> {
public:
    constexpr default_delete() noexcept = default;

    template <typename U,
              typename = enable_if_t<is_convertible<U (*)[], T (*)[]>::value>>
    default_delete(const default_delete<T[]>&) noexcept {
    }

    template <typename U,
              typename = enable_if_t<is_convertible<U (*)[], T (*)[]>::value>>
    void operator()(U* ptr) const {
        static_assert(sizeof(T) > 0, "delete a incomplete type, ub");
        delete[] ptr;
    }
};

template <typename T, typename D = default_delete<T>>
class unique_ptr;

template <typename T, typename Deleter>
class unique_ptr {
public:
    using pointer = T*;
    using element_type = T;
    using delete_type = Deleter;

private:
    extra::compress_pair<Deleter, pointer> mPair;

    pointer& getPtr() noexcept {
        return mPair.get_second();
    }

    const pointer& getPtr() const noexcept {
        return mPair.get_second();
    }

public:
    // (1)
    constexpr unique_ptr(std::nullptr_t p = nullptr) noexcept : mPair() {
        static_assert(is_default_constructible<Deleter>::value &&
                          !is_pointer<Deleter>::value,
                      "unique construct error");
    }

    // (2)
    explicit unique_ptr(pointer p) noexcept : mPair(Deleter(), p) {
        static_assert(is_default_constructible<Deleter>::value &&
                          !is_pointer<Deleter>::value,
                      "unique construct error");
    }

    // (3)
    unique_ptr(pointer p, conditional_t<is_reference<Deleter>::value, Deleter,
                                        const remove_reference_t<Deleter>&>
                              del) noexcept
        : mPair(tiny_stl::forward<decltype(del)>(del), p) {
    }

    // (4)
    unique_ptr(pointer p, remove_reference_t<Deleter>&& del) noexcept
        : mPair(tiny_stl::forward<decltype(del)>(del), p) {
        static_assert(!is_reference<Deleter>::value, "unique construct error");
    }

    // (5)
    unique_ptr(unique_ptr&& rhs) noexcept
        : mPair(tiny_stl::forward<delete_type>(rhs.get_deleter()),
                rhs.release()) {
    }

    // (6)
    template <typename U, typename D,
              typename =
                  enable_if_t<is_convertible<typename unique_ptr<U, D>::pointer,
                                             pointer>::value &&
                              !is_array<U>::value &&
                              ((is_reference<Deleter>::value &&
                                is_same<Deleter, D>::value) ||
                               (!is_reference<Deleter>::value &&
                                is_convertible<D, Deleter>::value))>>
    unique_ptr(unique_ptr<U, D>&& rhs) noexcept
        : mPair(tiny_stl::forward<D>(rhs.get_deleter()), rhs.release()) {
    }

    unique_ptr(const unique_ptr&) = delete;

    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr& operator=(unique_ptr&& rhs) noexcept {
        assert(this != tiny_stl::addressof(rhs));
        reset(rhs.release());
        get_deleter() = tiny_stl::forward<Deleter>(rhs.get_deleter());
        return *this;
    }

    template <typename U, typename D,
              typename =
                  enable_if_t<is_convertible<typename unique_ptr<U, D>::pointer,
                                             pointer>::value &&
                              is_assignable_v<Deleter&, D&&>>>
    unique_ptr& operator=(unique_ptr<U, D>&& rhs) noexcept {
        reset(rhs.release());
        get_deleter() = tiny_stl::forward<D>(rhs.get_deleter());
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    delete_type& get_deleter() noexcept {
        return mPair.get_first();
    }

    const delete_type& get_deleter() const noexcept {
        return mPair.get_first();
    }

    pointer get() const noexcept {
        return getPtr();
    }

    pointer release() noexcept {
        pointer ret = get();
        getPtr() = pointer();
        return ret;
    }

    void reset(pointer ptr = pointer()) noexcept {
        auto old = get();
        mPair.get_second() = ptr;
        if (old != nullptr)
            get_deleter()(old);
    }

    explicit operator bool() const noexcept {
        return get() != nullptr;
    }

    void swap(unique_ptr& rhs) noexcept {
        swapADL(getPtr(), rhs.getPtr());
        swapADL(get_deleter(), rhs.get_deleter());
    }

    ~unique_ptr() noexcept {
        if (get() != nullptr)
            this->get_deleter()(get());
    }

    add_lvalue_reference_t<T> operator*() const {
        assert(get() != nullptr);
        return *get();
    }

    pointer operator->() const noexcept {
        return getPtr();
    }
}; // unique_ptr<T, Deleter>

template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
public:
    using pointer = T*;
    using element_type = T;
    using delete_type = Deleter;

private:
    extra::compress_pair<Deleter, pointer> mPair;

    pointer& getPtr() noexcept {
        return mPair.get_second();
    }

    const pointer& getPtr() const noexcept {
        return mPair.get_second();
    }

    template <typename U, typename Is_nullptr = is_same<U, std::nullptr_t>>
    using EnableCtor = enable_if_t<
        is_same<U, pointer>::value || Is_nullptr::value ||
        (is_same<pointer, element_type*>::value && is_pointer<U>::value &&
         is_convertible<remove_pointer_t<U> (*)[], element_type (*)[]>::value)>;

public:
    // (1)
    constexpr unique_ptr(std::nullptr_t p = nullptr) noexcept : mPair() {
        static_assert(is_default_constructible<Deleter>::value &&
                          !is_pointer<Deleter>::value,
                      "unique construct error");
    }

    // (2)
    template <typename U, typename = EnableCtor<U>>
    explicit unique_ptr(U p) noexcept : mPair(delete_type(), p) {
        static_assert(is_default_constructible<Deleter>::value &&
                          !is_pointer<Deleter>::value,
                      "unique construct error");
    }

    // (3)
    template <typename U, typename = EnableCtor<U>>
    unique_ptr(U p, conditional_t<is_reference<Deleter>::value, Deleter,
                                  const remove_const_t<Deleter>&>
                        del) noexcept
        : mPair(tiny_stl::forward<decltype(del)>(del), p) {
    }

    // (4)
    template <typename U, typename = EnableCtor<U>>
    unique_ptr(U p, remove_reference_t<Deleter>&& del) noexcept
        : mPair(tiny_stl::forward<decltype(del)>(del), p) {
        static_assert(!is_reference<Deleter>::value,
                      "unique_ptr construct error");
    }

    // (5)
    unique_ptr(unique_ptr&& rhs) noexcept
        : mPair(tiny_stl::forward<Deleter>(rhs.get_deleter()), rhs.release()) {
    }

    // (6)
    template <
        typename U, typename D,
        typename = enable_if_t<
            is_array<U>::value && is_same<pointer, element_type*>::value &&
            is_same<typename unique_ptr<U, D>::pointer,
                    typename unique_ptr<U, D>::element_type*>::value &&
            is_convertible<typename unique_ptr<U, D>::element_type (*)[],
                           element_type (*)[]>::value &&
            ((is_reference<Deleter>::value && is_same<D, Deleter>::value) ||
             (!is_reference<Deleter>::value &&
              is_convertible<D, Deleter>::value))>>
    unique_ptr(unique_ptr<U, D>&& rhs) noexcept
        : mPair(tiny_stl::forward<D>(rhs.get_deleter()), rhs.release()) {
    }

    unique_ptr(const unique_ptr&) = delete;

    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr& operator=(unique_ptr&& rhs) noexcept {
        assert(this != tiny_stl::addressof(rhs));
        reset(rhs.release());
        get_deleter() = tiny_stl::move(rhs.get_deleter());
        return *this;
    }

    template <
        typename U, typename D,
        typename = enable_if_t<
            is_array<U>::value && is_same<pointer, element_type*>::value &&
            is_same<typename unique_ptr<U, D>::pointer,
                    typename unique_ptr<U, D>::element_type*>::value &&
            is_convertible<typename unique_ptr<U, D>::element_type (*)[],
                           element_type (*)[]>::value &&
            is_assignable<Deleter&, D&&>::value>>
    unique_ptr& operator=(unique_ptr<U, D>&& rhs) noexcept {
        reset(rhs.release());
        get_deleter() = tiny_stl::forward<D>(rhs.get_deleter());
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    delete_type& get_deleter() noexcept {
        return mPair.get_first();
    }

    const delete_type& get_deleter() const noexcept {
        return mPair.get_first();
    }

    pointer get() const noexcept {
        return getPtr();
    }

    pointer release() noexcept {
        pointer ret = get();
        getPtr() = pointer();
        return ret;
    }

    template <typename U, typename = EnableCtor<U, false_type>>
    void reset(U p) noexcept {
        pointer old = get();
        getPtr() = p;
        if (old != nullptr)
            get_deleter()(old);
    }

    void reset(std::nullptr_t) noexcept {
        reset(pointer());
    }

    explicit operator bool() const noexcept {
        return get() != nullptr;
    }

    void swap(unique_ptr& rhs) noexcept {
        swapADL(getPtr(), rhs.getPtr());
        swapADL(get_deleter(), rhs.get_deleter());
    }

    ~unique_ptr() noexcept {
        if (get() != nullptr)
            this->get_deleter()(get());
    }

    T& operator[](std::size_t i) const {
        return get()[i];
    }
}; // class unique_ptr<T[], Deleter>

template <typename T, typename... Args,
          typename = enable_if_t<!is_array<T>::value>>
inline unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(tiny_stl::forward<Args>(args)...));
}

template <typename T, typename = enable_if_t<is_array<T>::value &&
                                             std::extent<T>::value == 0>>
inline unique_ptr<T> make_unique(std::size_t size) {
    using E = std::remove_extent_t<T>;
    return unique_ptr<T>(new E[size]);
}

template <typename T, typename... Args,
          typename = enable_if_t<std::extent<T>::value != 0>>
void make_unique(Args&&...) = delete;

template <typename T, typename D>
struct hash<unique_ptr<T, D>> {
    using argument_type = unique_ptr<T, D>;
    using result_type = std::size_t;

    std::size_t operator()(const unique_ptr<T, D>& up) const noexcept {
        return hash<T>{}(*up);
    }
};

template <typename T1, typename D1, typename T2, typename D2>
inline bool operator==(const unique_ptr<T1, D1>& lhs,
                       const unique_ptr<T2, D2>& rhs) {
    return lhs.get() == rhs.get();
}

template <typename T1, typename D1, typename T2, typename D2>
inline bool operator!=(const unique_ptr<T1, D1>& lhs,
                       const unique_ptr<T2, D2>& rhs) {
    return !(lhs == rhs);
}

template <typename T1, typename D1, typename T2, typename D2>
inline bool operator<(const unique_ptr<T1, D1>& lhs,
                      const unique_ptr<T2, D2>& rhs) {
    using Common_t =
        typename std::common_type<typename unique_ptr<T1, D1>::pointer,
                                  typename unique_ptr<T2, D2>::pointer>::type;
    return less<Common_t>(lhs.get(), rhs.get());
}

template <typename T1, typename D1, typename T2, typename D2>
inline bool operator>(const unique_ptr<T1, D1>& lhs,
                      const unique_ptr<T2, D2>& rhs) {
    return rhs < lhs;
}

template <typename T1, typename D1, typename T2, typename D2>
inline bool operator<=(const unique_ptr<T1, D1>& lhs,
                       const unique_ptr<T2, D2>& rhs) {
    return !(rhs < lhs);
}

template <typename T1, typename D1, typename T2, typename D2>
inline bool operator>=(const unique_ptr<T1, D1>& lhs,
                       const unique_ptr<T2, D2>& rhs) {
    return !(lhs < rhs);
}

template <typename T, typename D>
inline bool operator==(const unique_ptr<T, D>& lhs, std::nullptr_t) noexcept {
    return !lhs;
}

template <typename T, typename D>
inline bool operator==(std::nullptr_t, const unique_ptr<T, D>& rhs) noexcept {
    return !rhs;
}

template <typename T, typename D>
inline bool operator!=(const unique_ptr<T, D>& lhs, std::nullptr_t) noexcept {
    return (bool)lhs;
}

template <typename T, typename D>
inline bool operator!=(std::nullptr_t, const unique_ptr<T, D>& rhs) noexcept {
    return (bool)rhs;
}

template <typename T, typename D>
inline bool operator<(const unique_ptr<T, D>& lhs, std::nullptr_t) {
    return less<typename unique_ptr<T, D>::pointer>()(lhs.get(), nullptr);
}

template <typename T, typename D>
inline bool operator<(std::nullptr_t, const unique_ptr<T, D>& rhs) {
    return less<typename unique_ptr<T, D>::pointer>()(nullptr, rhs.get());
}

template <typename T, typename D>
inline bool operator>(const unique_ptr<T, D>& lhs, std::nullptr_t) {
    return nullptr < lhs;
}

template <typename T, typename D>
inline bool operator>(std::nullptr_t, const unique_ptr<T, D>& rhs) {
    return rhs < nullptr;
}

template <typename T, typename D>
inline bool operator<=(const unique_ptr<T, D>& lhs, std::nullptr_t) {
    return !(nullptr < lhs);
}

template <typename T, typename D>
inline bool operator<=(std::nullptr_t, const unique_ptr<T, D>& rhs) {
    return !(rhs < nullptr);
}

template <typename T, typename D>
inline bool operator>=(const unique_ptr<T, D>& lhs, std::nullptr_t) {
    return !(lhs < nullptr);
}

template <typename T, typename D>
inline bool operator>=(std::nullptr_t, const unique_ptr<T, D>& rhs) {
    return !(nullptr < rhs);
}

template <typename T, typename D,
          typename = enable_if_t<is_swappable<D>::value>>
inline void swap(unique_ptr<T, D>& lhs, unique_ptr<T, D>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T>
class shared_ptr;

template <typename T>
class weak_ptr;

// reference MSVC implement

// reference count abstract base class
class RefCountBase {
protected:
    using AtomicCounterType = unsigned long;

private:
    virtual void destroyAux() noexcept = 0;
    virtual void deleteThis() noexcept = 0;

private:
    // assure use count operation is threads safe
    // why not use atomic
    std::atomic<AtomicCounterType> mUses;
    std::atomic<AtomicCounterType> mWeaks;

protected:
    // no-atomic initialization
    RefCountBase() : mUses(1), mWeaks(1) {
    }

public:
    virtual ~RefCountBase() noexcept {
    }

    bool increaseRefNotZero() {
        // increment use count if not zero, return true if successful
        for (;;) {
            // loop until state is known
            AtomicCounterType count = mUses.load();
            if (count == 0) {
                return false;
            }

            if (mUses.compare_exchange_strong(count, count + 1)) {
                return true;
            }
        }
    }

    long atomicIncrease(std::atomic<AtomicCounterType>& c) {
        return ++c;
    }

    long atomicDecrease(std::atomic<AtomicCounterType>& c) {
        return --c;
    }

    void increaseRef() {
        atomicIncrease(mUses);
    }

    void increaseWRef() {
        atomicIncrease(mWeaks);
    }

    void decreaseRef() {
        if (atomicDecrease(mUses) == 0) {
            // destroy managed resource, decrement the weak reference count
            destroyAux();
            decreaseWRef();
        }
    }

    void decreaseWRef() {
        if (atomicDecrease(mWeaks) == 0) {
            deleteThis();
        }
    }

    long useCount() const noexcept {
        return static_cast<long>(mUses);
    }

    virtual void* getDeleter(const std::type_info&) const noexcept {
        return nullptr;
    }
};

template <typename T>
class RefCount : public RefCountBase {
public:
    explicit RefCount(T* p) : RefCountBase(), mPtr(p) {
    }

private:
    virtual void destroyAux() noexcept override {
        delete mPtr;
    }

    virtual void deleteThis() noexcept override {
        delete this;
    }

private:
    T* mPtr;
};

// handle reference counting for object with deleter
template <typename T, typename D>
class RefCountResource : public RefCountBase {
public:
    RefCountResource(T p, D d) : RefCountBase(), mPair(tiny_stl::move(d), p) {
    }

    virtual void*
    getDeleter(const std::type_info& type) const noexcept override {
        if (type.hash_code() == typeid(D).hash_code()) {
            return const_cast<D*>(tiny_stl::addressof(mPair.get_first()));
        }

        return nullptr;
    }

private:
    virtual void destroyAux() noexcept override {
        mPair.get_first()(mPair.get_second());
    }

    virtual void deleteThis() noexcept override {
        delete this;
    }

private:
    extra::compress_pair<D, T> mPair;
};

// handle refernece counting for object with deleter and allocator
template <typename T, typename D, typename Alloc>
class RefCountResourceAlloc : public RefCountBase {
private:
    // allocator<RefCountResourceAlloc>
    using AllocType = GetAllocBindType<Alloc, RefCountResourceAlloc>;

public:
    RefCountResourceAlloc(T p, D d, const Alloc& alloc) : RefCountBase() {
    }

    virtual void*
    getDeleter(const std::type_info& type) const noexcept override {
        if (type == typeid(D)) {
            return const_cast<D*>(tiny_stl::addressof(mPair.get_first()));
        }

        return nullptr;
    }

private:
    virtual void destroyAux() noexcept override {
        mPair.get_first()(mPair.get_second().get_second());
    }

    virtual void deleteThis() noexcept override {
        AllocType alloc = mPair.get_second().get_first();
        allocator_traits<AllocType>::destroy(alloc, this);
        allocator_traits<AllocType>::deallocate(alloc, this, 1);
    }

private:
    extra::compress_pair<D, extra::compress_pair<AllocType, T>> mPair;
};

namespace {

template <typename T, typename = void>
struct CanEnableShared : false_type {};

template <typename F, typename Arg, typename = void>
struct IsFunctionObject : false_type {};

template <typename F, typename Arg>
struct IsFunctionObject<
    F, Arg, void_t<decltype(tiny_stl::declval<F>()(tiny_stl::declval<Arg>()))>>
    : true_type {};

// derived class is convertible to base class
template <typename T>
struct CanEnableShared<T, void_t<typename T::EsftUniqueType>>
    : is_convertible<remove_cv_t<T>*, typename T::EsftUniqueType*>::type {};

template <typename Other, typename U>
void enableSharedFromThisBase(const shared_ptr<Other>& sp, U* ptr, true_type) {
    if (ptr && ptr->mWptr.expired()) {
        ptr->mWptr =
            shared_ptr<remove_cv_t<U>>(sp, const_cast<remove_cv_t<U>*>(ptr));
    }
}

template <typename Other, typename U>
void enableSharedFromThisBase(const shared_ptr<Other>&, U*, false_type) {
}

} // namespace

template <typename Other, typename U>
void enableSharedFromThis(const shared_ptr<Other>& sp, U* ptr) {
    enableSharedFromThisBase(sp, ptr, CanEnableShared<U>{});
}

// base class for shared_ptr and weak_ptr
template <typename T>
class PtrBase {
public:
#ifdef TINY_STL_CXX17
    using element_type = remove_extent_t<T>;
#else
    using element_type = T;
#endif // TINY_STL_CXX17

public:
    long use_count() const noexcept {
        return mRep ? mRep->useCount() : 0;
    }

    template <typename U>
    bool owner_before(const PtrBase<U>& rhs) const noexcept {
        // compare the be managed object pointer
        return mPtr < rhs.mPtr;
    }

    element_type* get() const noexcept {
        return mPtr;
    }

    PtrBase(const PtrBase&) = delete;
    PtrBase& operator=(const PtrBase&) = delete;

protected:
    constexpr PtrBase() noexcept = default;
    ~PtrBase() = default;

    template <typename U>
    void constructMove(PtrBase<U>&& rhs) {
        // implement shared_ptr's move ctor and weak_ptr's move ctor
        mPtr = rhs.mPtr;
        mRep = rhs.mRep;

        rhs.mPtr = nullptr;
        rhs.mRep = nullptr;
    }

    template <typename U>
    void constructCopy(const shared_ptr<U>& rhs) {
        // implement shared_ptr's copy ctor
        if (rhs.mRep) {
            rhs.mRep->increaseRef();
        }

        mPtr = rhs.mPtr;
        mRep = rhs.mRep;
    }

    template <typename U>
    void constructFromAlias(const shared_ptr<U>& rhs, element_type* p) {
        // implement shared_ptr's aliasing ctor
        if (rhs.mRep) {
            rhs.mRep->increaseRef();
        }

        mPtr = p;
        mRep = rhs.mRep;
    }

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    bool constructFromWeak(const weak_ptr<U>& rhs) {
        if (rhs.mRep && rhs.mRep->increaseRefNotZero()) {
            mPtr = rhs.mPtr;
            mRep = rhs.mRep;
            return true;
        }

        return false;
    }

    void decreaseRef() {
        if (mRep) {
            mRep->decreaseRef();
        }
    }

    void decreaseWRef() {
        if (mRep) {
            mRep->decreaseWRef();
        }
    }

    void swapAux(PtrBase& rhs) noexcept {
        tiny_stl::swap(mPtr, rhs.mPtr);
        tiny_stl::swap(mRep, rhs.mRep);
    }

    void setPtrRep(element_type* ptr, RefCountBase* rep) {
        mPtr = ptr;
        mRep = rep;
    }

    template <typename U>
    void weaklyConstructFrom(const PtrBase<U>& rhs) {
        // implement weak_ptr's ctor
        if (rhs.mRep) {
            rhs.mRep->increaseWRef();
        }

        mPtr = rhs.mPtr;
        mRep = rhs.mRep;
    }

    template <typename U>
    friend class PtrBase;

private:
    element_type* mPtr{nullptr};
    RefCountBase* mRep{nullptr};

    template <typename D, typename U>
    friend D* get_deleter(const shared_ptr<U>& sp) noexcept;
};

template <typename T>
class shared_ptr : public PtrBase<T> {
public:
#ifdef TINY_STL_CXX17
    using weak_type = weak_ptr<T>;
#endif // TINY_STL_CXX17
    using element_type = typename PtrBase<T>::element_type;

    constexpr shared_ptr() noexcept {
    }

    constexpr shared_ptr(std::nullptr_t) noexcept {
    }

    // no C++17, in other words, no shared_ptr<T[]>
    // TODO: support C++17
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(U* ptr) {
        setPtr(ptr);
    }

    template <typename U, typename D,
              enable_if_t<conjunction_v<std::is_move_constructible<D>,
                                        is_convertible<U, T>,
                                        IsFunctionObject<D&, U*&>>,
                          int> = 0>
    shared_ptr(U* ptr, D d) {
        setPtrDel(ptr, tiny_stl::move(d));
    }

    template <typename D,
              enable_if_t<conjunction_v<std::is_move_constructible<D>,
                                        IsFunctionObject<D&, std::nullptr_t&>>,
                          int> = 0>
    shared_ptr(std::nullptr_t, D d) {
        setPtrDel(nullptr, tiny_stl::move(d));
    }

    template <typename U, typename D, typename Alloc,
              enable_if_t<conjunction_v<std::is_move_constructible<D>,
                                        is_convertible<U, T>,
                                        IsFunctionObject<D&, U*&>>,
                          int> = 0>
    shared_ptr(U* ptr, D d, Alloc alloc) {
        setPtrDelAlloc(ptr, tiny_stl::move(d), alloc);
    }

    template <typename D, typename Alloc,
              enable_if_t<conjunction_v<std::is_move_constructible<D>,
                                        IsFunctionObject<D&, std::nullptr_t*&>>,
                          int> = 0>
    shared_ptr(std::nullptr_t, D d, Alloc alloc) {
        setPtrDelAlloc(nullptr, tiny_stl::move(d), alloc);
    }

    template <typename U>
    shared_ptr(const shared_ptr<U>& rhs, element_type* ptr) noexcept {
        this->constructFromAlias(rhs, ptr);
    }

    shared_ptr(const shared_ptr& rhs) noexcept {
        this->constructCopy(rhs);
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr(const shared_ptr<U>& rhs) noexcept {
        this->constructCopy(rhs);
    }

    shared_ptr(shared_ptr&& rhs) noexcept {
        this->constructMove(tiny_stl::move(rhs));
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr(shared_ptr<U>&& rhs) noexcept {
        this->constructMove(tiny_stl::move(rhs));
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(const weak_ptr<U>& other) {
        if (!this->constructFromWeak(other)) {
            throw std::bad_weak_ptr{};
        }
    }

    template <
        typename U, typename D,
        enable_if_t<is_convertible_v<typename unique_ptr<U, D>::pointer, T*>,
                    int> = 0>
    shared_ptr(unique_ptr<U, D>&& other) {
        const U* ptr = other.get();

        using Deleter =
            conditional_t<is_reference_v<D>,
                          decltype(std::ref(other.get_deleter())), D>;

        if (ptr) {
            const auto pRcObj =
                new RefCountResource<U*, Deleter>{ptr, other.get_deleter()};
            setPtrRepAndEnableShared(ptr, pRcObj);
            other.release();
        }
    }

    ~shared_ptr() noexcept {
        this->decreaseRef();
    }

    shared_ptr& operator=(const shared_ptr& rhs) noexcept {
        shared_ptr{rhs}.swap(*this);
        return *this;
    }

    template <typename U>
    shared_ptr& operator=(const shared_ptr<U>& rhs) noexcept {
        shared_ptr{rhs}.swap(*this);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& rhs) noexcept {
        shared_ptr{tiny_stl::move(rhs)}.swap(*this);
        return *this;
    }

    template <typename U>
    shared_ptr& operator=(shared_ptr<U>&& rhs) noexcept {
        shared_ptr{tiny_stl::move(rhs)}.swap(*this);
        return *this;
    }

    template <typename U, typename D>
    shared_ptr& operator=(unique_ptr<U, D>&& other) {
        shared_ptr{tiny_stl::move(other)}.swap(*this);
        return *this;
    }

    void swap(shared_ptr& rhs) noexcept {
        this->swapAux(rhs);
    }

    void reset() noexcept {
        shared_ptr{}.swap(*this);
    }

    template <typename U>
    void reset(U* ptr) {
        shared_ptr{ptr}.swap(*this);
    }

    template <typename U, typename D>
    void reset(U* ptr, D d) {
        // d must be copy constructible
        shared_ptr{ptr, d}.swap(*this);
    }

    template <typename U, typename D, typename Alloc>
    void reset(U* ptr, D d, Alloc alloc) {
        shared_ptr{ptr, d, alloc}.swap(*this);
    }

    using PtrBase<T>::get;

    T& operator*() const noexcept {
        return *get();
    }

    T* operator->() const noexcept {
        return get();
    }

    // TODO: element_type& operator[](std::ptrdiff_t idx) (c++17)

    bool unique() const noexcept {
        return this->use_count() == 1;
    }

    explicit operator bool() const noexcept {
        return get() != nullptr;
    }

private:
    // TODO, support shared_ptr<T[]>
    template <typename U>
    void setPtr(U* ptr) {
        // strong exception guarantee
        try {
            setPtrRepAndEnableShared(ptr, new RefCount<U>(ptr));
        } catch (...) {
            delete ptr;
            throw;
        }
    }

    template <typename UptrOrNullptr, typename D>
    void setPtrDel(UptrOrNullptr ptr, D d) {
        try {
            setPtrRepAndEnableShared(ptr,
                                     new RefCountResource<UptrOrNullptr, D>);
        } catch (...) {
            d(ptr);
            throw;
        }
    }

    template <typename UptrOrNullptr, typename D, typename Alloc>
    void setPtrDelAlloc(UptrOrNullptr ptr, D d, Alloc alloc) {
        using RcObj = RefCountResourceAlloc<UptrOrNullptr, D, Alloc>;
        using Al_alloc =
            typename allocator_traits<Alloc>::template rebind_alloc<RcObj>;
        using Al_traits = allocator_traits<Al_alloc>;

        Al_alloc al_alloc{alloc};

        try {
            const auto pRcObj = Al_traits::allocate(al_alloc, 1);
            RcObj* const pref = tiny_stl::addressof(*pRcObj);
            try {
                Al_traits::construct(alloc, pref, ptr, tiny_stl::move(d),
                                     alloc);
                setPtrRepAndEnableShared(ptr, pref);
            } catch (...) {
                Al_traits::deallocate(al_alloc, pRcObj, 1);
                throw;
            }
        } catch (...) {
            d(ptr);
        }
    }

    template <typename U>
    void setPtrRepAndEnableShared(U* ptr, RefCountBase* rep) {
        this->setPtrRep(ptr, rep);
        enableSharedFromThis(*this, ptr);
    }

    void setPtrRepAndEnableShared(std::nullptr_t, RefCountBase* rep) {
        this->setPtrRep(nullptr, rep);
    }

    template <typename T0, typename... Args>
    friend shared_ptr<T0> make_shared(Args&&... args);

    template <typename T0, typename Alloc, typename... Args>
    friend shared_ptr<T0> allocate_shared(const Alloc& alloc, Args&&... args);

}; // class shared_ptr

namespace {

template <typename T>
class RefCountObj : public RefCountBase {
public:
    template <typename... Args>
    explicit RefCountObj(Args&&... args) noexcept : RefCountBase(), mStroage() {
        ::new (static_cast<void*>(&mStroage)) T(tiny_stl::forward<T>(args)...);
    }

    T* getPtr() {
        return reinterpret_cast<T*>(&mStroage);
    }

private:
    void destroyAux() noexcept override {
        getPtr()->~T();
    }

    void deleteThis() noexcept override {
        delete this;
    }

    std::aligned_union_t<1, T> mStroage;
};

} // namespace

template <typename T, typename... Args>
inline shared_ptr<T> make_shared(Args&&... args) {
    const auto pRcX = new RefCountObj<T>(tiny_stl::forward<Args>(args)...);

    shared_ptr<T> sp;
    sp.setPtrRepAndEnableShared(pRcX->getPtr(), pRcX);
    return sp;
}

namespace {

template <typename T, typename Alloc>
class RefCountObjAlloc : public RefCountBase {
public:
    template <typename... Args>
    explicit RefCountObjAlloc(const Alloc& al, Args&&... args) {
        ::new (static_cast<void*>(&mPair.get_second()))
            T(tiny_stl::forward<Args>(args)...);
    }

    T* getPtr() {
        return reinterpret_cast<T*>(&mPair.get_second());
    }

private:
    using AllocType = typename allocator_traits<Alloc>::template rebind_alloc<
        RefCountObjAlloc>;

    void destroyAux() noexcept override {
        getPtr()->~T();
    }

    void deleteThis() noexcept override {
        AllocType al = mPair.get_first();
        allocator_traits<AllocType>::destroy(al, this);
        allocator_traits<AllocType>::deallocate(al, this, 1);
    }

    extra::compress_pair<AllocType, std::aligned_union_t<1, T>> mPair;
};

} // namespace

template <typename T, typename Alloc, typename... Args>
shared_ptr<T> allocate_shared(const Alloc& alloc, Args&&... args) {
    using RcAlX = RefCountObjAlloc<T, Alloc>;
    using Al_alloc =
        typename allocator_traits<Alloc>::template rebind_alloc<RcAlX>;
    using Al_traits = allocator_traits<Al_alloc>;

    Al_alloc al{alloc};

    const auto pRcAlX = Al_traits::allocate(al, 1);

    try {
        Al_traits::construct(al, tiny_stl::addressof(*pRcAlX), alloc,
                             tiny_stl::forward<Args>(args)...);
    } catch (...) {
        Al_traits::deallocate(al, pRcAlX, 1);
    }

    shared_ptr<T> sp;
    sp.setPtrRepAndEnableShared(pRcAlX->getPtr(), tiny_stl::addressof(*pRcAlX));
    return sp;
}

template <typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& sp) noexcept {
    const auto ptr =
        static_cast<typename shared_ptr<T>::element_type*>(sp.get());
    return shared_ptr<T>(sp, ptr);
}

template <typename T, typename U>
shared_ptr<T> const_pointer_cast(const shared_ptr<U>& sp) noexcept {
    const auto ptr =
        const_cast<typename shared_ptr<T>::element_type*>(sp.get());
    return shared_ptr<T>(sp, ptr);
}

template <typename T, typename U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& sp) noexcept {
    const auto ptr =
        dynamic_cast<typename shared_ptr<T>::element_type*>(sp.get());
    if (ptr) {
        return shared_ptr<T>(sp, ptr);
    }

    // dynamic_cast failed and <new_type> is pointer type
    // return nullptr
    return shared_ptr<T>{};
}

#ifdef TINY_STL_CXX17
template <typename T, typename U>
shared_ptr<T> reinterpret_pointer_cast(const shared_ptr<U>& sp) noexcept {
    const auto ptr =
        reinterpret_cast<typename shared_ptr<T>::element_type*>(sp.get());
    return shared_ptr<T>(sp, ptr);
}
#endif

template <typename D, typename T>
D* get_deleter(const shared_ptr<T>& sp) noexcept {
    if (sp->_Rep) {
        return static_cast<D*>(sp._Rep->getDeleter(typeid(D)));
    }

    return nullptr;
}

// shared_ptr<T1> compare with shared_ptr<T2>
template <typename T1, typename T2>
bool operator==(const shared_ptr<T1>& lhs, const shared_ptr<T2>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
bool operator!=(const shared_ptr<T1>& lhs, const shared_ptr<T2>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T1, typename T2>
bool operator<(const shared_ptr<T1>& lhs, const shared_ptr<T2>& rhs) noexcept {
    return lhs.get() < rhs.get();
}

template <typename T1, typename T2>
bool operator>(const shared_ptr<T1>& lhs, const shared_ptr<T2>& rhs) noexcept {
    return rhs < lhs;
}

template <typename T1, typename T2>
bool operator<=(const shared_ptr<T1>& lhs, const shared_ptr<T2>& rhs) noexcept {
    return !(rhs < lhs);
}

template <typename T1, typename T2>
bool operator>=(const shared_ptr<T1>& lhs, const shared_ptr<T2>& rhs) noexcept {
    return !(lhs < rhs);
}

// shared_ptr<T> compare with nullptr
template <typename T>
bool operator==(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.get() == nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return rhs.get() == nullptr;
}

template <typename T>
bool operator!=(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.get() != nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return rhs.get() != nullptr;
}

template <typename T>
bool operator<(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return less<typename shared_ptr<T>::element_type>()(lhs.get(), nullptr);
}

template <typename T>
bool operator<(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return less<typename shared_ptr<T>::element_type>()(nullptr, rhs.get());
}

template <typename T>
bool operator>(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return nullptr < lhs;
}

template <typename T>
bool operator>(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return rhs < nullptr;
}

template <typename T>
bool operator<=(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return !(nullptr < lhs);
}

template <typename T>
bool operator<=(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return !(rhs < nullptr);
}

template <typename T>
bool operator>=(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return !(lhs < nullptr);
}

template <typename T>
bool operator>=(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return !(nullptr < rhs);
}

template <typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const shared_ptr<T>& ptr) {
    return os << ptr.get();
}

template <typename T>
void swap(shared_ptr<T>& lhs, shared_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T>
class weak_ptr : public PtrBase<T> {
public:
    constexpr weak_ptr() noexcept {
    }

    weak_ptr(const weak_ptr& rhs) noexcept {
        this->weaklyConstructFrom(rhs);
    }

    // TODO
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const weak_ptr<U>& rhs) noexcept {
        this->weaklyConstructFrom(rhs.lock());
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const shared_ptr<U>& rhs) noexcept {
        this->weaklyConstructFrom(rhs);
    }

    weak_ptr(weak_ptr&& rhs) noexcept {
        this->constructMove(tiny_stl::move(rhs));
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(weak_ptr<U>&& rhs) noexcept {
        this->weaklyConstructFrom(rhs.lock());
        rhs.reset();
    }

    ~weak_ptr() noexcept {
        this->decreaseWRef();
    }

    weak_ptr& operator=(const weak_ptr& rhs) noexcept {
        weak_ptr(rhs).swap(*this);
        return *this;
    }

    template <typename U>
    weak_ptr& operator=(const weak_ptr<U>& rhs) noexcept {
        weak_ptr(rhs).swap(*this);
        return *this;
    }

    template <typename U>
    weak_ptr& operator=(const shared_ptr<U>& rhs) noexcept {
        weak_ptr(rhs).swap(*this);
        return *this;
    }

    weak_ptr& operator=(weak_ptr&& rhs) noexcept {
        weak_ptr(tiny_stl::move(rhs)).swap(*this);
        return *this;
    }

    template <typename U>
    weak_ptr& operator=(weak_ptr<U>&& rhs) noexcept {
        weak_ptr(tiny_stl::move(rhs)).swap(*this);
        return *this;
    }

    void reset() noexcept {
        weak_ptr().swap(*this);
    }

    void swap(weak_ptr& rhs) noexcept {
        this->swapAux(rhs);
    }

    // use_count() in base class

    // checking validity of the pointer
    bool expired() const noexcept {
        return this->use_count() == 0;
    }

    shared_ptr<T> lock() const noexcept {
        return expired() ? shared_ptr<T>{} : shared_ptr<T>{*this};
    }
};

template <typename T>
void swap(weak_ptr<T>& lhs, weak_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T>
class enable_shared_from_this {
public:
    shared_ptr<T> shared_from_this() {
        return shared_ptr<T>(mWptr);
    }

    shared_ptr<const T> shared_from_this() const {
        return shared_ptr<const T>(mWptr);
    }

    weak_ptr<T> weak_from_this() {
        return mWptr;
    }

    weak_ptr<const T> weak_from_this() const {
        return mWptr;
    }

protected:
    constexpr enable_shared_from_this() noexcept : mWptr() {
    }

    enable_shared_from_this(const enable_shared_from_this&) noexcept : mWptr() {
    }

    enable_shared_from_this&
    operator=(const enable_shared_from_this&) noexcept {
        return *this;
    }

    ~enable_shared_from_this() = default;

private:
    // helper type to determine if TYPE has inherited enable_from_this
    using EsftUniqueType = enable_shared_from_this;

    template <typename Other, typename U>
    friend void enableSharedFromThis(const shared_ptr<Other>& sp, U* ptr);

    mutable weak_ptr<T> mWptr;
};

} // namespace tiny_stl
