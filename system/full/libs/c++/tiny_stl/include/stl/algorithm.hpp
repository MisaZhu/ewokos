// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cassert>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <random>

#include "functional.hpp"
#include "iterator.hpp"

namespace tiny_stl {

template <typename InIter, typename UnaryPred>
inline bool all_of(InIter first, InIter last, UnaryPred pred) {
    for (; first != last; ++first)
        if (!pred(*first))
            return false;

    return true;
}

template <typename InIter, typename UnaryPred>
inline bool any_of(InIter first, InIter last, UnaryPred pred) {
    for (; first != last; ++first)
        if (pred(*first))
            return true;

    return false;
}

template <typename InIter, typename UnaryPred>
inline bool none_of(InIter first, InIter last, UnaryPred pred) {
    for (; first != last; ++first)
        if (pred(*first))
            return false;

    return true;
}

template <typename InIter, typename UnaryFunc>
inline UnaryFunc for_each(InIter first, InIter last, UnaryFunc f) {
    for (; first != last; ++first)
        f(*first);

    return tiny_stl::move(f);
}

template <typename InIter, typename UnaryPred>
inline typename iterator_traits<InIter>::difference_type
count_if(InIter first, InIter last, UnaryPred pred) {
    typename iterator_traits<InIter>::difference_type c = 0;
    for (; first != last; ++first)
        if (pred(*first))
            ++c;

    return c;
}

template <typename InIter, typename T>
inline typename iterator_traits<InIter>::difference_type
count(InIter first, InIter last, const T& val) {
    return count_if(first, last, [&val](const auto& v) { return v == val; });
}

template <typename InIter1, typename InIter2, typename BinPred>
inline pair<InIter1, InIter2> mismatch(InIter1 first1, InIter1 last1,
                                       InIter2 first2, BinPred pred) {
    while (first1 != last1 && pred(*first1, *first2)) {
        ++first1;
        ++first2;
    }

    return make_pair(first1, first2);
}

template <typename InIter1, typename InIter2>
inline pair<InIter1, InIter2> mismatch(InIter1 first1, InIter1 last1,
                                       InIter2 first2) {
    while (first1 != last1 && *first1 != *first2) {
        ++first1;
        ++first2;
    }

    return tiny_stl::make_pair(first1, first2);
}

template <typename InIter1, typename InIter2, typename BinPred>
inline pair<InIter1, InIter2> mismatch(InIter1 first1, InIter1 last1,
                                       InIter2 first2, InIter2 last2,
                                       BinPred pred) {
    while (first1 != last1 && first2 != last2 && pred(*first1, *first2)) {
        ++first1;
        ++first2;
    }

    return tiny_stl::make_pair(first1, first2);
}

template <typename InIter1, typename InIter2>
inline pair<InIter1, InIter2> mismatch(InIter1 first1, InIter1 last1,
                                       InIter2 first2, InIter2 last2) {
    while (first1 != last1 && first2 != last2 && *first1 != *first2) {
        ++first1;
        ++first2;
    }

    return tiny_stl::make_pair(first1, first2);
}

template <typename InIter, typename T>
inline InIter find(InIter first, InIter last, const T& val) {
    for (; first != last; ++first)
        if (*first == val)
            return first;

    return last;
}

template <typename InIter, typename UnaryPred>
inline InIter find_if(InIter first, InIter last, UnaryPred pred) {
    for (; first != last; ++first)
        if (pred(*first))
            return first;

    return last;
}

template <typename InIter, typename UnaryPred>
inline InIter find_if_not(InIter first, InIter last, UnaryPred pred) {
    for (; first != last; ++first)
        if (!pred(*first))
            return first;

    return last;
}

template <typename FwdIter, typename T>
struct FillMemsetIsSafeHelper {
    using ValueType = typename iterator_traits<FwdIter>::value_type;
    using type = typename conjunction<
        is_pointer<FwdIter>,
        disjunction<conjunction<IsCharacter<T>, IsCharacter<ValueType>>,
                    conjunction<is_same<bool, T>, is_same<bool, ValueType>>>>::
        type;
};

namespace details {

template <typename FwdIter, typename T>
inline typename FillMemsetIsSafeHelper<FwdIter, T>::type
fillMemsetIsSafe(const FwdIter&, const T&) {
    return {};
}

template <typename OutIter, typename Diff, typename T>
inline OutIter fillNHelper(OutIter dst, Diff n, const T& val,
                           true_type /* fill memset is safe*/) {
    if (n > 0) {
        std::memset(dst, val, n);
        return dst + n;
    }
    return dst;
}

template <typename OutIter, typename Diff, typename T>
inline OutIter fillNHelper(OutIter dst, Diff n, const T& val,
                           false_type /* fill memset is unsafe */) {
    for (; n > 0; --n, ++dst)
        *dst = val;
    return dst;
}

template <typename FwdIter, typename T>
inline void fillHelper(FwdIter first, FwdIter last, const T& val, true_type) {
    std::memset(first, last - first, val);
}

template <typename FwdIter, typename T>
inline void fillHelper(FwdIter first, FwdIter last, const T& val, false_type) {
    for (; first != last; ++first)
        *first = val;
}

} // namespace details

template <typename OutIter, typename Diff, typename T>
inline OutIter fill_n(OutIter dst, Diff n, const T& val) {
    return details::fillNHelper(dst, n, val,
                                details::fillMemsetIsSafe(dst, val));
}

template <typename FwdIter, typename T>
inline void fill(FwdIter first, FwdIter last, const T& val) {
    details::fillHelper(first, last, val,
                        details::fillMemsetIsSafe(first, val));
}

template <typename FwdIter, typename Func>
inline void generate(FwdIter first, FwdIter last, Func f) {
    for (; first != last; ++first) {
        *first = f();
    }
}

template <typename OutIter, typename Size, typename Func>
inline OutIter generate_n(OutIter first, Size n, Func f) {
    for (Size i = 0; i < n; ++i) {
        *first++ = f();
    }

    return first;
}

template <typename InIter, typename OutIter, typename UnaryOp>
inline OutIter transform(InIter first1, InIter last1, OutIter dstFirst,
                         UnaryOp op) {
    for (; first1 != last1; ++first1, ++dstFirst)
        *dstFirst = op(*first1);

    return dstFirst;
}

template <typename InIter, typename OutIter, typename BinOp>
inline OutIter transform(InIter first1, InIter last1, InIter first2,
                         OutIter dstFirst, BinOp op) {
    for (; first1 != last1; ++first1, ++first2, ++dstFirst)
        *dstFirst = op(*first1, *first2);

    return dstFirst;
}

template <typename InIter, typename OutIter, typename UnaryPred>
inline OutIter copy_if(InIter first, InIter last, OutIter dst, UnaryPred pred) {
    for (; first != last; ++first)
        if (pred(*first))
            *dst++ = *first;

    return dst; // new last
}

template <typename InIter, typename OutIter>
inline OutIter copy(InIter first, InIter last, OutIter dst) {
    for (; first != last; ++first)
        *dst++ = *first;

    return dst;
}

template <typename InIter, typename Size, typename OutIter>
inline OutIter copy_n(InIter src, Size count, OutIter dst) {
    if (count > 0) {
        for (Size i = 0; i < count; ++i)
            *dst = *src;
    }

    return dst;
}

template <typename BidIter1, typename BidIter2>
inline BidIter2 copy_backward(BidIter1 first, BidIter1 last, BidIter2 dstLast) {
    for (; first != last;)
        *(--dstLast) = *(--last);

    return dstLast;
}

template <typename InIter, typename OutIter>
inline OutIter move(InIter first, InIter last, OutIter dstFirst) {
    for (; first != last;)
        *(dstFirst++) = tiny_stl::move(*first++);

    return dstFirst;
}

template <typename BidIter1, typename BidIter2>
inline BidIter2 move_backward(BidIter1 first, BidIter1 last, BidIter2 dstLast) {
    for (; first != last;)
        *(--dstLast) = tiny_stl::move(*(--last));

    return dstLast;
}

template <typename FwdIter1, typename FwdIter2>
inline FwdIter2 swap_ranges(FwdIter1 first1, FwdIter1 last1, FwdIter2 first2) {
    for (; first1 != last1; ++first1, ++first2)
        tiny_stl::iter_swap(first1, first2);
    return first2;
}

template <typename InIter1, typename InIter2, typename BinPred>
inline bool equal(InIter1 first1, InIter1 last1, InIter2 first2, BinPred pred) {
    for (; first1 != last1; ++first1, ++first2)
        if (!pred(*first1, *first2))
            return false;

    return true;
}

template <typename InIter1, typename InIter2>
inline bool equal(InIter1 first1, InIter1 last1, InIter2 first2) {
    return tiny_stl::equal(first1, last1, first2, tiny_stl::equal_to<>());
}

template <typename InIter1, typename InIter2, typename BinPred>
inline bool equal(InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
                  BinPred pred) {
    // for input iterator
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        if (!pred(*first1, *first2))
            return false;

    return first1 == last1 && first2 == last2;
}

template <typename InIter1, typename InIter2>
inline bool equal(InIter1 first1, InIter1 last1, InIter2 first2,
                  InIter2 last2) {
    return tiny_stl::equal(first1, last1, first2, last2,
                           tiny_stl::equal_to<>{});
}

template <typename FwdIter, typename Cmp>
constexpr FwdIter min_element(FwdIter first, FwdIter last, Cmp cmp) {
    if (first == last)
        return last;
    FwdIter max_iter = first++;

    for (; first != last; ++first)
        if (cmp(*first, *max_iter))
            max_iter = first;

    return max_iter;
}

template <typename FwdIter>
constexpr FwdIter min_element(FwdIter first, FwdIter last) {
    return tiny_stl::min_element(first, last, tiny_stl::less<>{});
}

template <typename T, typename Cmp>
constexpr const T& min(const T& a, const T& b, Cmp cmp) {
    return cmp(a, b) ? a : b;
}

template <typename T>
constexpr const T& min(const T& a, const T& b) {
    return a < b ? a : b;
}

template <typename T, typename Cmp>
constexpr T min(std::initializer_list<T> ilist, Cmp cmp) {
    return *tiny_stl::min_element(ilist.begin(), ilist.end(), cmp);
}

template <typename T>
constexpr T min(std::initializer_list<T> ilist) {
    return tiny_stl::min(ilist, tiny_stl::less<>{});
}

template <typename FwdIter, typename Cmp>
constexpr FwdIter max_element(FwdIter first, FwdIter last, Cmp cmp) {
    if (first == last)
        return last;
    FwdIter max_iter = first++;

    for (; first != last; ++first)
        if (cmp(*max_iter, *first))
            max_iter = first;

    return max_iter;
}

template <typename FwdIter>
constexpr FwdIter max_element(FwdIter first, FwdIter last) {
    return tiny_stl::max_element(first, last, tiny_stl::less<>{});
}

template <typename T, typename Cmp>
constexpr const T& max(const T& a, const T& b, Cmp cmp) {
    return cmp(a, b) ? b : a;
}

template <typename T>
constexpr const T& max(const T& a, const T& b) {
    return a < b ? b : a;
}

template <typename T, typename Cmp>
constexpr T max(std::initializer_list<T> ilist, Cmp cmp) {
    return *tiny_stl::max_element(ilist.begin(), ilist.end(), cmp);
}

template <typename T>
constexpr T max(std::initializer_list<T> ilist) {
    return tiny_stl::max(ilist, tiny_stl::less<>{});
}

template <typename T, typename Compare>
inline pair<const T&, const T&> minmax(const T& a, const T& b, Compare cmp) {
    return cmp(a, b) ? pair<const T&, const T&>(a, b)
                     : pair<const T&, const T&>(b, a);
}

template <typename T>
inline pair<const T&, const T&> minmax(const T& a, const T& b) {
    return tiny_stl::minmax(a, b, tiny_stl::less<>{});
}

template <typename FwdIter, typename Compare>
inline pair<FwdIter, FwdIter> minmax_element(FwdIter first, FwdIter last,
                                             Compare cmp) {
    // return { The smallest first element, The largest last element }
    pair<FwdIter, FwdIter> ret{first, first};

    if (first == last) // empty range
        return ret;

    if (++first == last) // there is only one element
        return ret;

    if (cmp(*first, *ret.first)) // compare *(first+1) with result
        ret.first = first;
    else
        ret.second = first;

    // compare [first+2, last) with result
    while (++first != last) {
        FwdIter iter = first;
        if (++first == last) {
            if (cmp(*iter, *ret.first))
                ret.first = iter;
            else if (!(cmp(*iter, *ret.second)))
                ret.second = iter;
            break;
        } else { // *first is smaller if cmp is less
            if (cmp(*first, *iter)) {
                if (cmp(*first, *ret.first))
                    ret.first = first;
                if (!cmp(*iter, *ret.second))
                    ret.second = iter;
            } else {
                if (cmp(*iter, *ret.first))
                    ret.first = iter;
                if (!cmp(*first, *ret.second))
                    ret.second = first;
            }
        }
    }

    return ret;
}

template <typename FwdIter>
inline pair<FwdIter, FwdIter> minmax_element(FwdIter first, FwdIter last) {
    return tiny_stl::minmax_element(first, last, tiny_stl::less<>{});
}

template <typename T>
inline pair<T, T> minmax(std::initializer_list<T> ilist) {
    auto p = tiny_stl::minmax_element(ilist.begin(), ilist.end(),
                                      tiny_stl::less<>{});
    return tiny_stl::make_pair(*p.first, *p.second);
}

template <typename T, typename Compare>
inline pair<T, T> minmax(std::initializer_list<T> ilist, Compare cmp) {
    auto p = tiny_stl::minmax_element(ilist.begin(), ilist.end(), cmp);
    return tiny_stl::make_pair(*p.first, *p.second);
}

namespace details {

template <typename InIter1, typename InIter2, typename BinPred>
inline bool lexicographicalCompareHelper(InIter1 first1, InIter1 last1,
                                         InIter2 first2, InIter2 last2,
                                         BinPred pred) {
    for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
        if (pred(*first1, *first2))
            return true;
        if (pred(*first2, *first1))
            return false;
    }

    // If one range is a prefix of another, the shorter
    // range is lexicographically less than the other.
    return (first1 == last1 && first2 != last2);
}

} // namespace details

template <typename InIter1, typename InIter2, typename BinPred>
inline bool lexicographical_compare(InIter1 first1, InIter1 last1,
                                    InIter2 first2, InIter2 last2,
                                    BinPred pred) {
    return details::lexicographicalCompareHelper(first1, last1, first2, last2,
                                                 pred);
}

template <typename InIter1, typename InIter2>
inline bool lexicographical_compare(InIter1 first1, InIter1 last1,
                                    InIter2 first2, InIter2 last2) {
    return tiny_stl::lexicographical_compare(first1, last1, first2, last2,
                                             less<>());
}

template <typename FwdIter>
inline FwdIter rotate(FwdIter first, FwdIter mid, FwdIter last) {
    // return first + (last - mid)
    if (first == mid)
        return last;
    if (mid == last)
        return first;

    FwdIter next = mid;
    do { // left
        tiny_stl::iter_swap(first++, next++);
        if (first == mid)
            mid = next;
    } while (next != last);

    FwdIter ret = first; // this_first = old_first + (last - mid)

    // right
    for (next = mid; next != last;) {
        tiny_stl::iter_swap(first++, next++);
        if (first == mid)
            mid = next;
        else if (next == last)
            next = mid;
    }

    return ret;
}

template <typename BidIter>
inline void reverse(BidIter first, BidIter last) {
    for (; first != last && first != --last; ++first)
        tiny_stl::iter_swap(first, last);
}

namespace details {

template <typename RanIter, typename Diff, typename T, typename Cmp>
inline void pushHeapHelper(RanIter first, Diff hole, Diff top, T&& val,
                           Cmp& cmp) {
    Diff i = (hole - 1) / 2;
    for (; top < hole && cmp(*(first + i), val); i = (hole - 1) / 2) {
        // parent < val
        *(first + hole) = tiny_stl::move(*(first + i)); // move down parent
        hole = i;                                       // update the hole
    }

    *(first + hole) = tiny_stl::move(val); // move val to the hole
}

template <typename RanIter, typename Diff, typename T, typename Cmp>
inline void adjustHeapHelper(RanIter first, Diff hole, Diff len, T&& val,
                             Cmp& cmp) {
    Diff top = hole;                // 0
    Diff rightChild = hole * 2 + 2; // init right child to 2

    // have a right child
    while (rightChild < len) {
        if (*(first + rightChild) <
            *(first + (rightChild - 1))) // left child > right child
            --rightChild;
        *(first + hole) = tiny_stl::move(
            *(first + rightChild));      // move the bigger one to the hole
        hole = rightChild;               // update the hole
        rightChild = rightChild * 2 + 2; // update the right child
    }

    // only left child
    if (rightChild == len) {
        // move the left child to the hole
        *(first + hole) = tiny_stl::move(*(first + (rightChild - 1)));

        hole = rightChild - 1;
    }

    // val is the origin last element. move it to hole
    details::pushHeapHelper(first, hole, top, tiny_stl::move(val), cmp);
}

template <typename RanIter, typename T, typename Cmp>
inline void popHeapHelper(RanIter first, RanIter last, RanIter dst, T&& val,
                          Cmp& cmp) {
    using Diff = typename iterator_traits<RanIter>::difference_type;
    *dst = tiny_stl::move(*first); // move the first to tail, hole is 0

    adjustHeapHelper(first, static_cast<Diff>(0),
                     static_cast<Diff>(last - first), tiny_stl::move(val), cmp);
}

} // namespace details

template <typename RanIter, typename Cmp>
inline void push_heap(RanIter first, RanIter last, Cmp cmp) {
    using Diff = typename iterator_traits<RanIter>::difference_type;
    Diff count = last - first;
    if (count >= 2) {
        auto val = tiny_stl::move(*--last);
        details::pushHeapHelper(first, --count, static_cast<Diff>(0),
                                tiny_stl::move(val), cmp);
    }
}

// O(logn)
template <typename RanIter>
inline void push_heap(RanIter first, RanIter last) {
    tiny_stl::push_heap(first, last, tiny_stl::less<>{});
}

template <typename RanIter, typename Cmp>
inline void pop_heap(RanIter first, RanIter last, Cmp cmp) {
    --last;
    auto val = tiny_stl::move(*last);
    details::popHeapHelper(first, last, last, tiny_stl::move(val), cmp);
}

template <typename RanIter>
inline void pop_heap(RanIter first, RanIter last) {
    tiny_stl::pop_heap(first, last, tiny_stl::less<>{});
}

template <typename RanIter, typename Cmp>
inline void sort_heap(RanIter first, RanIter last, Cmp cmp) {
    for (; last - first > 1; --last)
        tiny_stl::pop_heap(first, last);
}

template <typename RanIter>
inline void sort_heap(RanIter first, RanIter last) {
    tiny_stl::sort_heap(first, last, tiny_stl::less<>{});
}

template <typename RanIter, typename Cmp>
inline void make_heap(RanIter first, RanIter last, Cmp cmp) {
    if (last - first < 2)
        return;

    using Diff = typename iterator_traits<RanIter>::difference_type;
    Diff len = last - first;
    Diff parent = (len - 2) / 2;

    while (true) {
        auto val = tiny_stl::move(*(first + parent));
        details::adjustHeapHelper(first, parent, len, val, cmp);
        if (parent-- == 0)
            return;
    }
}

template <typename RanIter>
inline void make_heap(RanIter first, RanIter last) {
    tiny_stl::make_heap(first, last, tiny_stl::less<>{});
}

template <typename RanIter, typename Cmp>
inline RanIter is_heap_until(RanIter first, RanIter last, Cmp cmp) {
    using Diff = typename iterator_traits<RanIter>::difference_type;

    Diff len = last - first;
    if (len >= 2)
        for (Diff offset = 1; offset < len; ++offset)
            if (cmp(*(first + (offset - 1) / 2),
                    *(first + offset))) // parent < child
                return first + offset;

    return last;
}

template <typename RanIter, typename Cmp>
inline RanIter is_heap_until(RanIter first, RanIter last) {
    return tiny_stl::is_heap_until(first, last, tiny_stl::less<>{});
}

template <typename RanIter, typename Cmp>
inline bool is_heap(RanIter first, RanIter last, Cmp cmp) {
    return tiny_stl::is_heap_until(first, last, cmp) == last;
}

template <typename RanIter>
inline bool is_heap(RanIter first, RanIter last) {
    return tiny_stl::is_heap(first, last, tiny_stl::less<>{});
}

template <typename FwdIter, typename Cmp>
inline FwdIter is_sorted_until(FwdIter first, FwdIter last, Cmp cmp) {
    if (first != last)
        for (FwdIter next = first; ++next != last; ++first)
            if (cmp(*next, *first)) // *next < *cur
                return next;

    return last;
}

template <typename FwdIter>
inline FwdIter is_sorted_until(FwdIter first, FwdIter last) {
    return tiny_stl::is_sorted_until(first, last, tiny_stl::less<>{});
}

template <typename FwdIter, typename Cmp>
inline bool is_sorted(FwdIter first, FwdIter last, Cmp cmp) {
    return tiny_stl::is_sorted_until(first, last, cmp) == last;
}

template <typename FwdIter>
inline bool is_sorted(FwdIter first, FwdIter last) {
    return tiny_stl::is_sorted_until(first, last) == last;
}

namespace details {

// insert_sort
template <typename RanIter, typename Compare>
inline void insertSort(RanIter first, RanIter last, Compare& cmp) {
    if (first == last)
        return;

    for (RanIter i = first + 1; i != last; ++i) {
        auto key = tiny_stl::move(*i);
        RanIter j = i;
        for (; j > first && cmp(key, (*(j - 1))); --j) {
            *j = tiny_stl::move(*(j - 1));
        }

        *j = tiny_stl::move(key);
    }
}

// quick_sort
template <typename RanIter>
inline auto getRandom(RanIter first, RanIter last) {
    using Diff = typename iterator_traits<RanIter>::difference_type;
    static std::mt19937 e;
    std::uniform_int_distribution<Diff> u{0, (last - first - 1)};

    return u(e);
}

template <typename RanIter, typename Compare>
inline RanIter quickSortPartition(RanIter first, RanIter last, Compare& cmp) {
    using Diff = typename iterator_traits<RanIter>::difference_type;
    Diff randomPos = getRandom(first, last);
    tiny_stl::iter_swap(first + randomPos, last - 1);
    auto key = *(last - 1);

    RanIter i = first;
    for (RanIter j = first; j < last - 1; ++j) {
        if (cmp(*j, key)) {
            tiny_stl::iter_swap(i++, j);
        }
    }
    tiny_stl::iter_swap(i, last - 1);

    return i;
}

static const std::ptrdiff_t INSERT_SORT_MAX = 32;

template <typename RanIter, typename Compare>
inline void quickSort(RanIter first, RanIter last, IterDiffType<RanIter> diff,
                      Compare& cmp) {
    IterDiffType<RanIter> count = last - first;
    if (count > INSERT_SORT_MAX && diff > 0) {
        RanIter miditer = quickSortPartition(first, last, cmp);

        diff = (diff >> 1) + (diff >> 2);

        quickSort(first, miditer, diff, cmp);
        first = miditer;

        quickSort(miditer + 1, last, diff, cmp);
        last = miditer + 1;
    }

    if (count > INSERT_SORT_MAX) {
        tiny_stl::make_heap(first, last, cmp);
        tiny_stl::sort_heap(first, last, cmp);
    }

    else if (count >= 2) {
        insertSort(first, last, cmp);
    }
}

} // namespace details

template <typename RanIter, typename Compare>
inline void sort(RanIter first, RanIter last, Compare cmp) {
    if (last - first - 1 > 0) {
        details::quickSort(first, last, last - first, cmp);
    }
}

template <typename RanIter>
inline void sort(RanIter first, RanIter last) {
    sort(first, last, tiny_stl::less<>{});
}

template <typename FwdIter, typename T, typename Compare>
inline FwdIter lower_bound(FwdIter first, FwdIter last, const T& val,
                           Compare cmp) {
    assert(is_sorted(first, last, cmp));

    FwdIter ret = first;
    using Diff = typename iterator_traits<FwdIter>::difference_type;
    Diff size = tiny_stl::distance(first, last);
    Diff step = 0;

    while (size > 0) {
        ret = first;
        step = size >> 1;
        tiny_stl::advance(ret, step);

        if (!cmp(*ret, val)) { // left
            size = step;
        } else { // right
            first = ++ret;
            size -= (step + 1);
        }
    }

    return ret;
}

template <typename FwdIter, typename T>
inline FwdIter lower_bound(FwdIter first, FwdIter last, const T& val) {
    return tiny_stl::lower_bound(first, last, val, tiny_stl::less<>{});
}

template <typename FwdIter, typename T, typename Compare>
inline FwdIter upper_bound(FwdIter first, FwdIter last, const T& val,
                           Compare cmp) {
    assert(is_sorted(first, last, cmp));

    FwdIter ret = first;
    using Diff = typename iterator_traits<FwdIter>::difference_type;
    Diff size = tiny_stl::distance(first, last);
    Diff step = 0;

    while (size > 0) {
        ret = first;
        step = size >> 1;
        tiny_stl::advance(ret, step);
        if (cmp(val, *ret)) { // left
            size = step;
        } else { // right
            first = ++ret;
            size -= (step + 1);
        }
    }

    return ret;
}

template <typename FwdIter, typename T>
inline FwdIter upper_bound(FwdIter first, FwdIter last, const T& val) {
    return tiny_stl::upper_bound(first, last, val, tiny_stl::less<>{});
}

template <typename FwdIter, typename T, typename Compare>
inline FwdIter binary_search(FwdIter first, FwdIter last, const T& val,
                             Compare cmp) {
    first = tiny_stl::lower_bound(first, last, val, cmp);
    return (!(first == last)) && !(cmp(val, *first));
}

template <typename FwdIter, typename T>
inline FwdIter binary_search(FwdIter first, FwdIter last, const T& val) {
    first = tiny_stl::lower_bound(first, last, val);
    return (!(first == last)) && !(val < *first);
}

template <typename FwdIter, typename T, typename Compare>
inline pair<FwdIter, FwdIter> equal_range(FwdIter first, FwdIter last,
                                          const T& val, Compare cmp) {
    return tiny_stl::make_pair(lower_bound(first, last, val, cmp),
                               upper_bound(first, last, val, cmp));
}

template <typename FwdIter, typename T>
inline pair<FwdIter, FwdIter> equal_range(FwdIter first, FwdIter last,
                                          const T& val) {
    return tiny_stl::make_pair(lower_bound(first, last, val),
                               upper_bound(first, last, val));
}

} // namespace tiny_stl
