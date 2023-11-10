// Copyright (C) 2021 syn1w
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <chrono>
#include <climits>
#include <ctime>
#include <iostream>

#include "stl/array.hpp"
#include "stl/cow_string.hpp"
#include "stl/deque.hpp"
#include "stl/forward_list.hpp"
#include "stl/iterator.hpp"
#include "stl/list.hpp"
#include "stl/map.hpp"
#include "stl/memory.hpp"
#include "stl/queue.hpp"
#include "stl/rbtree.hpp"
#include "stl/set.hpp"
#include "stl/stack.hpp"
#include "stl/string.hpp"
#include "stl/string_view.hpp"
#include "stl/tuple.hpp"
#include "stl/type_traits.hpp"
#include "stl/unordered_map.hpp"
#include "stl/unordered_set.hpp"
#include "stl/vector.hpp"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define UNIT_TEST_BASE(equality, expect, actual)                               \
    do {                                                                       \
        test_count++;                                                          \
        if (equality)                                                          \
            test_pass++;                                                       \
        else {                                                                 \
            std::cerr << __FILE__ << ":" << __LINE__                           \
                      << ":  except: " << expect << " actual: " << actual      \
                      << std::endl;                                            \
            main_ret = 1;                                                      \
        }                                                                      \
    } while (0)

#define UNIT_TEST(expect, actual)                                              \
    UNIT_TEST_BASE((expect) == (actual), (expect), (actual))

template <typename Con>
void print_elements(const Con& c) {
    for (const auto& x : c)
        std::cout << x << " ";
    std::cout << "\n";
}

template <typename ConWithPair>
void print_element_with_pair(const ConWithPair& c) {
    for (const auto& x : c)
        std::cout << "[" << x.first << ", " << x.second << "]" << std::endl;
    std::cout << "\n";
}

void testUtility() {
    int i = 31, j = 42;
    tiny_stl::swap(i, j);
    UNIT_TEST(31, j);
    UNIT_TEST(42, i);

    int arr0[5] = {1, 2, 3, 4, 5};
    int arr1[5] = {5, 4, 3, 2, 1};
    tiny_stl::swap(arr0, arr1);
    UNIT_TEST(5, arr0[0]);

    tiny_stl::pair<std::int32_t, bool> p0;
    UNIT_TEST(0, p0.first);
    UNIT_TEST(false, p0.second);
    tiny_stl::pair<std::int32_t, float> p1(42, 3.2f);
    UNIT_TEST(3.2f, p1.second);
    // tiny_stl::pair<std::int32_t, float> p2(tiny_stl::move(p1));

    auto p = tiny_stl::make_pair(1, "pair");
    UNIT_TEST(1, p.first);
    UNIT_TEST(std::string{"pair"}, p.second);
    UNIT_TEST(1, tiny_stl::get<0>(p));
    const auto cp = p;
    UNIT_TEST(1, tiny_stl::get<int>(cp));
    UNIT_TEST(3.14, tiny_stl::get<1>(tiny_stl::make_pair(42, 3.14)));
}

void testTypeTraits() {
    using namespace tiny_stl;
    UNIT_TEST(true, is_array_v<int[]>);
    UNIT_TEST(true, is_array_v<int[2]>);
    UNIT_TEST(false, is_array_v<int*>);

    class C {};
    struct S {};
    enum class E {};
    union U {};
    UNIT_TEST(true, is_class_v<C>);
    UNIT_TEST(true, is_class_v<S>);
    UNIT_TEST(false, is_class_v<E>);
    UNIT_TEST(false, is_class_v<U>);
    UNIT_TEST(true, is_union_v<U>);

    UNIT_TEST(true, (is_same_v<int, int>));
    UNIT_TEST(true, (is_same_v<int, remove_const_t<const int>>));
    UNIT_TEST(true, (is_same_v<int*, remove_const_t<int* const>>));
    UNIT_TEST(true, (is_same_v<int[3], remove_const_t<const int[3]>>));
    UNIT_TEST(false, (is_same_v<int*, remove_const_t<const int*>>));
    UNIT_TEST(true, is_function_v<int(int)>);
    UNIT_TEST(false, is_function_v<int>);
    UNIT_TEST(true, (is_function_v<decltype(printf)>));
    UNIT_TEST(true, is_void_v<void>);
    UNIT_TEST(true, (is_same_v<add_lvalue_reference_t<int>, int&>));
    UNIT_TEST(true, (is_same_v<add_rvalue_reference_t<int>, int&&>));
    UNIT_TEST(true, is_floating_point_v<double>);
    UNIT_TEST(true, is_integral_v<std::int32_t>);
    UNIT_TEST(true, (is_same_v<int*, add_pointer_t<int>>));
    UNIT_TEST(true, (is_same_v<add_pointer_t<int(int)>,
                               tiny_stl::add_pointer_t<int(int)>>));
}

template <typename T>
bool is_equal(const T& lhs, const T& rhs) {
    return lhs == rhs;
}

void testMemory() {
    std::string str = "abcd";
    char buffer[5];
    tiny_stl::uninitialized_copy(str.c_str(), str.c_str() + str.size() + 1,
                                 buffer);
    UNIT_TEST(str, std::string(buffer));
    memset(buffer, 0, sizeof(buffer));
    tiny_stl::uninitialized_copy_n(str.c_str(), str.size() + 1, buffer);
    UNIT_TEST(str, std::string(buffer));
    memset(buffer, 0, sizeof(buffer));
    tiny_stl::uninitialized_fill(buffer, buffer + sizeof(buffer), 'a');
    buffer[4] = '\0';
    std::string str1 = "aaaa";
    UNIT_TEST(str1, std::string(buffer));
    memset(buffer, 0, sizeof(buffer));
    tiny_stl::uninitialized_fill_n(buffer, sizeof(buffer), 'a');
    buffer[4] = '\0';
    UNIT_TEST(str1, std::string(buffer));

    tiny_stl::cow_wstring wstr = L"abcd";
    wchar_t wbuffer[5];
    tiny_stl::uninitialized_copy(wstr.c_str(), wstr.c_str() + wstr.size() + 1,
                                 wbuffer);
    UNIT_TEST(true, is_equal(wstr, tiny_stl::cow_wstring(wbuffer)));

    using Deleter = void (*)(int*);
    tiny_stl::unique_ptr<int, Deleter> up(new int(42),
                                          [](int* p) { delete p; });
    UNIT_TEST(42, *up);

    tiny_stl::unique_ptr<int> up1(new int(42));
    UNIT_TEST(42, *up1);

    tiny_stl::unique_ptr<int> up2(new int(3));

    up1 = tiny_stl::move(up2);
    UNIT_TEST(3, *up1);
    UNIT_TEST(true, up2.get() == nullptr);
    tiny_stl::unique_ptr<int, void (*)(int*)> up3(new int[10],
                                                  [](int* p) { delete[] p; });
    tiny_stl::unique_ptr<int[]> up4(new int[10]);
    tiny_stl::unique_ptr<int[]> up5(new int[20]);
    up5 = tiny_stl::move(up4);

    tiny_stl::shared_ptr<int> sp0;
    UNIT_TEST(false, static_cast<bool>(sp0));
    UNIT_TEST(0, sp0.use_count());
    UNIT_TEST(true, sp0 == nullptr);

    tiny_stl::shared_ptr<int> sp1{new int(42)};
    UNIT_TEST(true, static_cast<bool>(sp1));
    UNIT_TEST(1, sp1.use_count());
    UNIT_TEST(true, sp1.unique());
    UNIT_TEST(42, *sp1);
    UNIT_TEST(false, sp0 == sp1);

    tiny_stl::shared_ptr<int> sp2{sp1};
    UNIT_TEST(2, sp1.use_count());
    UNIT_TEST(2, sp2.use_count());
    UNIT_TEST(42, *sp2);
    UNIT_TEST(true, sp1 == sp2);
    UNIT_TEST(42, *sp2.get());

    auto sp3 = tiny_stl::make_shared<int>(42);
    UNIT_TEST(42, *sp3);

    auto sp4 = tiny_stl::allocate_shared<int>(tiny_stl::allocator<int>{}, 42);
    UNIT_TEST(42, *sp4);
}

void testAlgorithm() {
    tiny_stl::vector<int> v1 = {1, 2, 3, 4, 2};
    UNIT_TEST(true, tiny_stl::is_sorted(v1.begin(), v1.end() - 1));
    UNIT_TEST(true,
              (--v1.end()) == tiny_stl::is_sorted_until(v1.begin(), v1.end()));
    std::initializer_list<int> list1 = {-1, -2, 0, 1, 2, 3, 4, 5, -4};
    UNIT_TEST(-4, tiny_stl::min(list1));
    UNIT_TEST(5, tiny_stl::max(list1));
    UNIT_TEST(4, tiny_stl::min(5, 4));
    UNIT_TEST(5, tiny_stl::max(4, 5));

    tiny_stl::vector<int> v2 = {12, 8, 7, 5, 6, 4, 2};
    v2.push_back(15);
    tiny_stl::push_heap(v2.begin(), v2.end());
    UNIT_TEST(15, v2.front());
    UNIT_TEST(8, v2.size());
    tiny_stl::pop_heap(v2.begin(), v2.end());
    v2.pop_back();
    UNIT_TEST(12, v2.front());
    UNIT_TEST(7, v2.size());
    tiny_stl::sort_heap(v2.begin(), v2.end());
    UNIT_TEST(true, tiny_stl::is_sorted(v2.begin(), v2.end()));
    tiny_stl::make_heap(v2.begin(), v2.end());
    UNIT_TEST(true, tiny_stl::is_heap(v2.begin(), v2.end()));

    tiny_stl::vector<int> v = {3, 4, 2, 3, 5, 5, 3, 5, 2};
    UNIT_TEST(3, tiny_stl::count(v.begin(), v.end(), 5));

    int a = 4, b = 2;
    auto p = tiny_stl::minmax(a, b);
    UNIT_TEST(2, p.first);
    UNIT_TEST(4, p.second);

    auto pi = tiny_stl::minmax_element(v.begin(), v.end());
    UNIT_TEST(4, *(pi.first - 1));
    UNIT_TEST(2, *(pi.second + 1));

    tiny_stl::vector<int> vs;

    srand(static_cast<unsigned>(time(0)));
    for (int i = 0; i < 32; ++i) {
        vs.push_back(rand());
    }

    tiny_stl::sort(vs.begin(), vs.end());
    UNIT_TEST(true, tiny_stl::is_sorted(vs.begin(), vs.end()));

#if 0
    tiny_stl::vector<int> bigNums(100'000'000);
    for (int i = 0; i < 100'000'000; ++i)
    {
        bigNums[i] = rand();
    }

    auto begin1 = std::chrono::high_resolution_clock::now();
    tiny_stl::sort(bigNums.begin(), bigNums.end());
    auto end1 = std::chrono::high_resolution_clock::now();

    std::cout << "time: " << ((end1 - begin1).count() / 1000000.0) << "ms" << std::endl;

    UNIT_TEST(true, tiny_stl::is_sorted(bigNums.begin(), bigNums.end()));
#endif
}

void testArray() {
    tiny_stl::array<std::int32_t, 10> arr;
    arr.assign(42);
    UNIT_TEST(42, arr[9]);

    tiny_stl::array<std::int32_t, 5> arr1{1, 2, 3, 4, 5};
    tiny_stl::array<std::int32_t, 5> re_arr1{5, 4, 3, 2, 1};
    UNIT_TEST(2, arr1.at(1));
    UNIT_TEST(3, arr1[2]);

    std::int32_t i = 1;
    for (auto x : arr1) {
        UNIT_TEST(i, x);
        ++i;
    }

    i = 1;
    for (auto riter = re_arr1.rbegin(); riter != re_arr1.rend(); ++riter) {
        UNIT_TEST(i, *riter);
        ++i;
    }

    tiny_stl::array<std::int32_t, 5> arr2{1, 2, 3, 4, 5};
    tiny_stl::array<std::int32_t, 5> arr3{3, 2, 1};
    UNIT_TEST(true, arr1 == arr2);
    UNIT_TEST(true, arr2 != arr3);
    UNIT_TEST(5, *arr2.rbegin());

    arr2.swap(arr3);
    UNIT_TEST(3, arr2[0]);
    UNIT_TEST(0, arr2[3]);
    UNIT_TEST(0, arr2[4]);
    UNIT_TEST(5, tiny_stl::tuple_size<decltype(arr2)>::value);
    UNIT_TEST(true,
              (tiny_stl::is_same_v<tiny_stl::tuple_element_t<0, decltype(arr2)>,
                                   std::int32_t>));

    tiny_stl::array<int, 5> arr4;
    UNIT_TEST(5, arr4.end() - arr4.begin());
}

void testVector() {
    tiny_stl::vector<int> v;
    UNIT_TEST(0, v.size());
    tiny_stl::vector<int> v1(3, 42);
    UNIT_TEST(3, v1.size());
    UNIT_TEST(3, v1.capacity());
    UNIT_TEST(42, *v1.begin());
    UNIT_TEST(42, v1[2]);
    v1.reserve(2);
    UNIT_TEST(3, v1.capacity());
    v1.reserve(4);
    UNIT_TEST(3, v1.size());
    UNIT_TEST(4, v1.capacity());
    UNIT_TEST(42, v1[2]);
    v1.shrink_to_fit();
    UNIT_TEST(3, v1.capacity());
    tiny_stl::vector<int> v2(3);
    UNIT_TEST(3, v2.size());
    UNIT_TEST(3, v2.capacity());
    UNIT_TEST(0, *v2.begin());
    v1[0] = 4;
    UNIT_TEST(4, v1[0]);
    v1.swap(v2); // v1 = { 0, 0, 0 }, v2 = { 4, 42, 42 };
    UNIT_TEST(0, v1[2]);
    UNIT_TEST(4, v2[0]);
    v2.emplace_back(42); // v2 = {4, 42, 42, 42};
    UNIT_TEST(4, v2.size());
    UNIT_TEST(42, v2[3]);
    v2.push_back(42); // v2 = {4, 42, 42, 42, 42};
    UNIT_TEST(42, v2[4]);
    tiny_stl::vector<int> v4(v2.begin(), v2.end());
    UNIT_TEST(5, v4.size());
    UNIT_TEST(42, v4.back());
    UNIT_TEST(4, v4.front());
    tiny_stl::vector<int> v5 = {1, 2, 3, 4, 5};
    UNIT_TEST(5, v5.size());
    UNIT_TEST(5, v5.capacity());
    UNIT_TEST(5, v5[4]);
    tiny_stl::vector<int> v6 = v5;
    UNIT_TEST(5, v6.size());
    UNIT_TEST(5, v6.capacity());
    UNIT_TEST(5, v6[4]);
    tiny_stl::vector<tiny_stl::vector<int>> v7(3, tiny_stl::vector<int>(4));
    UNIT_TEST(3, v7.size());
    UNIT_TEST(4, v7[0].size());
    tiny_stl::vector<tiny_stl::vector<int>> v8(3, tiny_stl::vector<int>(4));
    UNIT_TEST(3, v8.size());
    UNIT_TEST(4, v8[0].size());
    tiny_stl::vector<int> v9{tiny_stl::move(v5)}; // v9 = move(v5);
    UNIT_TEST(5, v9.size());
    UNIT_TEST(5, v9.capacity());
    UNIT_TEST(5, v9[4]);
    v9 = v1;
    UNIT_TEST(3, v9.size());
    UNIT_TEST(0, v9[0]);
    tiny_stl::vector<int> v10;
    v10 = tiny_stl::move(v9); // v10 = move(v9);
    UNIT_TEST(3, v10.size());
    UNIT_TEST(0, v10[0]);
    tiny_stl::vector<int> v11{1, 2, 3};
    v11.emplace(v11.end(), 4);
    UNIT_TEST(4, v11.size());
    UNIT_TEST(4, v11[3]);
    v11.emplace(v11.begin(), 0);
    UNIT_TEST(5, v11.size());
    UNIT_TEST(0, v11[0]);
    UNIT_TEST(1, v11[1]);
    v11.insert(v11.begin(), 3, 0);
    UNIT_TEST(8, v11.size());
    UNIT_TEST(0, v11[1]);

    tiny_stl::vector<tiny_stl::vector<int>> v12 = {
        {1, 2, 3, 4}, {2, 3, 4, 5}, {3, 4, 5, 6}};
    v12.insert(v12.begin() + 1, {{1, 2, 3}, {4, 5, 6, 7}, {1, 2, 3}});
    UNIT_TEST(6, v12.size());
    UNIT_TEST(4, v12[0].size());
    UNIT_TEST(3, v12[1].size());
    tiny_stl::vector<int> v13 = {1, 2, 3, 4, 5, 6};
    auto iter1 = v13.erase(v13.begin());
    UNIT_TEST(5, v13.size());
    UNIT_TEST(2, v13[0]);
    UNIT_TEST(2, *iter1);
    auto iter2 = v13.erase(v13.begin() + 2, v13.begin() + 4);
    UNIT_TEST(3, v13.size());
    UNIT_TEST(6, v13[2]);
    UNIT_TEST(6, *iter2);
    tiny_stl::vector<int> v14 = {1, 2, 3, 4};
    tiny_stl::vector<int> v15 = {2, 3, 4, 5};
    UNIT_TEST(false, v14 == v15);
    UNIT_TEST(true, v14 < v15);

    tiny_stl::vector<int> v16;
    v16.reserve(10);
    v16.assign(10, 42);
    UNIT_TEST(10, v16.size());
    UNIT_TEST(42, v16.front());
    UNIT_TEST(42, v16.back());
}

void testList() {
    tiny_stl::list<int> l1;
    UNIT_TEST(0, l1.size());
    UNIT_TEST(true, l1.empty());
    l1.emplace_back(0);
    UNIT_TEST(1, l1.size());
    UNIT_TEST(0, *l1.begin());
    l1.push_back(1);
    UNIT_TEST(2, l1.size());
    UNIT_TEST(1, *(++l1.begin()));
    auto iter1 = l1.insert(l1.begin(), -1);
    UNIT_TEST(3, l1.size());
    UNIT_TEST(-1, l1.front());
    UNIT_TEST(1, l1.back());
    UNIT_TEST(-1, *iter1);
    iter1 = l1.erase(++l1.begin());
    UNIT_TEST(2, l1.size());
    UNIT_TEST(1, *++l1.begin());
    UNIT_TEST(1, *iter1);
    tiny_stl::list<int> l2(5, 2);
    UNIT_TEST(5, l2.size());
    UNIT_TEST(2, l2.front());
    UNIT_TEST(2, l2.back());
    l2.insert(l2.begin(), 0);
    UNIT_TEST(6, l2.size());
    UNIT_TEST(0, l2.front());
    auto iter2 = l2.insert(--(--l2.end()), 4);
    UNIT_TEST(7, l2.size());
    UNIT_TEST(4, *iter2);
    l2.resize(5);
    UNIT_TEST(5, l2.size());
    UNIT_TEST(4, l2.back());
    l2.resize(7);
    UNIT_TEST(7, l2.size());
    UNIT_TEST(0, l2.back());
    tiny_stl::list<int> l3(tiny_stl::move(l2));
    UNIT_TEST(0, l2.size());
    UNIT_TEST(7, l3.size());
    l3.assign(5, 3);
    UNIT_TEST(5, l3.size());
    UNIT_TEST(3, l3.front());
    std::initializer_list<int> ilist1{1, 2, 3, 4};
    l2.assign(ilist1);
    UNIT_TEST(4, l2.size());
    UNIT_TEST(1, l2.front());
    UNIT_TEST(4, l2.back());
    l3.assign(l2.begin(), l2.end());
    UNIT_TEST(4, l3.size());
    UNIT_TEST(1, l3.front());
    UNIT_TEST(4, l3.back());
    l2.assign(10, 2);
    l3 = tiny_stl::move(l2);
    UNIT_TEST(10, l3.size());
    UNIT_TEST(2, l3.front());
    l2.assign(5, 3);
    l2.swap(l3);
    UNIT_TEST(10, l2.size());
    UNIT_TEST(5, l3.size());
    tiny_stl::list<tiny_stl::list<int>> l4 = {{1, 2}, {3, 4, 5}, {6, 7, 8, 9}};

    UNIT_TEST(3, l4.size());
    UNIT_TEST(2, l4.front().size());
    UNIT_TEST(1, l4.front().front());
    UNIT_TEST(3, (++l4.begin())->size());
    UNIT_TEST(4, l4.back().size());

    tiny_stl::list<int> l5 = {1, 2, 3, 4, 5};
    tiny_stl::list<int> l6 = {6, 7, 8, 9, 10};
    l5.splice(l5.end(), l6);
    UNIT_TEST(10, l5.size());
    UNIT_TEST(0, l6.size());
    UNIT_TEST(10, l5.back());
    l6 = {10, 11, 12, 13, 14, 15};
    l5.splice(l5.end(), l6, ++l6.begin());
    UNIT_TEST(5, l6.size());
    UNIT_TEST(11, l5.size());
    UNIT_TEST(11, l5.back());
    l5.splice(l5.end(), l6, ++l6.begin(), l6.end());
    UNIT_TEST(15, l5.size());
    UNIT_TEST(15, l5.back());
    UNIT_TEST(1, l6.size());
    l5 = {1, 2, 4, 6, 9};
    l6 = {0, 2, 5, 5, 7};
    l5.merge(l6);

    UNIT_TEST(10, l5.size());
    UNIT_TEST(0, l5.front());
    UNIT_TEST(7, *(--(--l5.end())));
    UNIT_TEST(true, tiny_stl::is_sorted(l5.begin(), l5.end()));
    l5 = {1, 2, 3};
    l5.reserve();
    UNIT_TEST(1, l5.back());
    UNIT_TEST(2, *++l5.begin());
    UNIT_TEST(3, l5.front());
    tiny_stl::list<int> l7 = {3, 4, 2, 1, 5, 6, 0, 7};
    l7.sort();
    UNIT_TEST(true, tiny_stl::is_sorted(l7.begin(), l7.end()));
}

void testForwardList() {
    tiny_stl::forward_list<int> fl0;
    UNIT_TEST(true, fl0.empty());
    auto iter0 = fl0.insert_after(fl0.before_begin(), 1);
    UNIT_TEST(1, *iter0);
    UNIT_TEST(1, fl0.front());
    iter0 = fl0.insert_after(fl0.before_begin(), 2, 0);
    UNIT_TEST(0, *iter0);
    UNIT_TEST(0, fl0.front());
    UNIT_TEST(1, *++iter0);
    std::initializer_list<int> ilist = {1, 2, 3, 4};
    iter0 = fl0.insert_after(fl0.before_begin(), ilist.begin(), ilist.end());
    UNIT_TEST(4, *iter0);
    UNIT_TEST(0, *++iter0);
    iter0 = fl0.erase_after(fl0.before_begin());
    UNIT_TEST(2, fl0.front());
    UNIT_TEST(2, *iter0);
    iter0 = fl0.erase_after(fl0.before_begin(), ++(++fl0.begin()));
    UNIT_TEST(4, *iter0);
    UNIT_TEST(4, fl0.front());
    fl0.resize(2);

    tiny_stl::forward_list<int> fl1(10, 42);
    UNIT_TEST(42, fl1.front());
    tiny_stl::forward_list<int> fl2 = {1, 2, 3, 4, 5};
    UNIT_TEST(1, fl2.front());
    tiny_stl::forward_list<int> fl3 = tiny_stl::move(fl2);
    UNIT_TEST(true, fl2.empty());
    UNIT_TEST(1, fl3.front());
    tiny_stl::forward_list<int> fl4 = fl3;
    UNIT_TEST(1, fl4.front());
    fl4 = tiny_stl::move(fl1);
    UNIT_TEST(true, fl1.empty());
    UNIT_TEST(42, fl4.front());

    tiny_stl::forward_list<int> fl5 = {1, 3, 4, 6, 7};
    tiny_stl::forward_list<int> fl6 = {2, 5, 8, 9};
    fl5.merge(fl6);
    UNIT_TEST(true, is_sorted(fl5.begin(), fl5.end()));
    tiny_stl::forward_list<int> fl7 = {1, 3, 4};
    tiny_stl::forward_list<int> fl8 = {1, 2, 3, 4};
    fl7.splice_after(fl7.begin(), fl8, fl8.begin());
    // print_elements(fl7);
    // print_elements(fl8);
    tiny_stl::forward_list<int> fl9 = {3, 4, 3, 5, 6, 7};
    fl9.remove_if([](const int& val) { return val > 5; });
    // print_elements(fl9);
    fl9.remove(3);
    UNIT_TEST(4, fl9.front());
    // print_elements(fl9);
    tiny_stl::forward_list<int> fl10 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    fl10.reverse();
    UNIT_TEST(10, fl10.front());
    // print_elements(fl10);

    tiny_stl::forward_list<int> fl11 = {1, 2, 3, 3, 3, 4, 2, 6,
                                        6, 7, 2, 3, 4, 8, 8};
    fl11.unique();
    // print_elements(fl11);
    fl11.sort();
    UNIT_TEST(true, is_sorted(fl11.begin(), fl11.end()));
    // print_elements(fl11);

    tiny_stl::forward_list<int> fl12 = {4, 6, 3, 4, 1, 2};
    fl12.sort();
    UNIT_TEST(true, is_sorted(fl12.begin(), fl12.end()));
    // print_elements(t);
}

void testDeque() {
    tiny_stl::deque<int> d1;
    UNIT_TEST(true, d1.empty());
    UNIT_TEST(0, d1.size());

    tiny_stl::deque<int> d2(10, 42);
    UNIT_TEST(10, d2.size());
    UNIT_TEST(42, d2[0]);
    UNIT_TEST(42, d2.front());
    UNIT_TEST(42, d2.back());

    tiny_stl::deque<int> d3(1024);
    UNIT_TEST(1024, d3.size());
    for (int i = 0; i < 256; ++i)
        d3.emplace_back(i);
    UNIT_TEST(1280, d3.size());
    UNIT_TEST(255, d3.back());
    for (int i = 256; i < 512; ++i)
        d3.emplace_front(i);
    UNIT_TEST(1536, d3.size());
    UNIT_TEST(511, d3[0]);
    UNIT_TEST(256, d3[255]);
    d3.pop_back();
    UNIT_TEST(1535, d3.size());
    UNIT_TEST(254, d3.back());
    d3.pop_front();
    UNIT_TEST(1534, d3.size());
    UNIT_TEST(510, d3.front());
    for (int i = 0; i < 150; ++i)
        d3.pop_back();
    UNIT_TEST(1384, d3.size());
    for (int i = 0; i < 150; ++i)
        d3.pop_front();
    UNIT_TEST(1234, d3.size());

    tiny_stl::deque<int> d4 = {1, 2, 3, 4};
    UNIT_TEST(4, d4.size());
    UNIT_TEST(1, d4.front());
    UNIT_TEST(4, d4.back());
    auto iter1 = d4.erase(d4.begin());
    UNIT_TEST(3, d4.size());
    UNIT_TEST(2, d4.front());
    UNIT_TEST(2, *iter1);
    d4.shrink_to_fit();
    UNIT_TEST(3, d4.size());
    UNIT_TEST(2, d4.front());
    d4.emplace(d4.begin(), 1);
    UNIT_TEST(4, d4.size());
    UNIT_TEST(1, d4.front());
    d4.insert(d4.begin(), d2.begin(), d2.begin() + 3);
    UNIT_TEST(7, d4.size());
    UNIT_TEST(42, d4[0]);
    UNIT_TEST(42, d4[1]);
    UNIT_TEST(42, d4[2]);
    UNIT_TEST(1, d4[3]);
    d4.insert(d4.end(), 3, 33);
    UNIT_TEST(10, d4.size());
    UNIT_TEST(33, d4.back());
    d4.insert(d4.end(), 30, 42);
    UNIT_TEST(40, d4.size());
    UNIT_TEST(42, d4.back());

    tiny_stl::deque<int> d5;
    d5 = d4;
    UNIT_TEST(40, d5.size());
    UNIT_TEST(42, d5.back());
    d5.assign(10, 42);
    UNIT_TEST(10, d5.size());
    UNIT_TEST(42, d5[0]);
    std::initializer_list<int> ilist = {1, 2, 3, 4, 5};
    d5.assign(ilist);
    UNIT_TEST(5, d5.size());
    UNIT_TEST(1, d5[0]);
    UNIT_TEST(5, d5[4]);
    d5.resize(10);
    UNIT_TEST(10, d5.size());
    d5.resize(4);
    UNIT_TEST(4, d5.size());
}

void testAdaptor() {
    tiny_stl::stack<int> s1;
    UNIT_TEST(true, s1.empty());
    UNIT_TEST(0, s1.size());
    s1.push(42);
    UNIT_TEST(42, s1.top());
    s1.push(132);
    UNIT_TEST(132, s1.top());
    UNIT_TEST(2, s1.size());
    for (int i = 0; i < 1000; ++i)
        s1.push(i);
    UNIT_TEST(1002, s1.size());
    UNIT_TEST(999, s1.top());
    s1.emplace(10);
    UNIT_TEST(10, s1.top());
    UNIT_TEST(1003, s1.size());
    for (int i = 0; i < 200; ++i)
        s1.pop();
    UNIT_TEST(803, s1.size());

    tiny_stl::queue<int> q1;
    UNIT_TEST(0, q1.size());
    UNIT_TEST(true, q1.empty());
    for (int i = 0; i < 1000; ++i)
        q1.push(i);
    UNIT_TEST(1000, q1.size());
    UNIT_TEST(false, q1.empty());
    UNIT_TEST(0, q1.front());
    UNIT_TEST(999, q1.back());
    for (int i = 0; i < 100; ++i)
        q1.emplace(i);
    UNIT_TEST(1100, q1.size());
    UNIT_TEST(99, q1.back());
    for (int i = 0; i < 100; ++i)
        q1.emplace(42);
    UNIT_TEST(1200, q1.size());
    for (int i = 0; i < 200; ++i)
        q1.pop();
    UNIT_TEST(1000, q1.size());
    UNIT_TEST(200, q1.front());

    tiny_stl::vector<int> v = {1, 2, 3, 4, 5};
    tiny_stl::priority_queue<int> pq1{tiny_stl::less<int>{}, v};

    UNIT_TEST(5, pq1.top());
    UNIT_TEST(5, pq1.size());
    pq1.pop();
    UNIT_TEST(4, pq1.top());
    pq1.push(10);
    UNIT_TEST(10, pq1.top());
}

void testStringView() {
    tiny_stl::string_view str0;
    UNIT_TEST(true, str0.empty());

    tiny_stl::string_view str1 = "abcd";
    UNIT_TEST(4, str1.size());
    UNIT_TEST('a', *str1.begin());
    UNIT_TEST('a', str1.front());
    UNIT_TEST('d', str1.back());
    UNIT_TEST('b', *(str1.begin() + 1));
    UNIT_TEST('d', *(str1.end() - 1));
    UNIT_TEST('b', *++str1.begin());
    UNIT_TEST('d', *--str1.end());
    UNIT_TEST('c', str1[2]);
    UNIT_TEST('c', str1.at(2));
    UNIT_TEST(false, str1.empty());

#if 0
    try
    {
        UNIT_TEST('\0', str1.at(4));
    }
    catch (std::out_of_range& e)
    {
        std::cout << e.what() << std::endl;
    }

    std::cout << str1 << std::endl;
#endif // switch

    tiny_stl::string_view str2 = "12345";
    str1.swap(str2);
    UNIT_TEST(5, str1.size());
    UNIT_TEST('1', str1.front());

    tiny_stl::string_view str3 = "   trim me    ";
    str3.remove_prefix(3);
    UNIT_TEST('t', str3.front());
    str3.remove_suffix(4);
    UNIT_TEST('e', str3.back());

    char buf[32] = {};
    std::size_t copyLen = str1.copy(buf, 3, 1);
    UNIT_TEST(true, copyLen == 3);
    UNIT_TEST('2', buf[0]);
    UNIT_TEST('3', buf[1]);
    UNIT_TEST('4', buf[2]);

    memset(buf, 0, sizeof(buf));
    copyLen = str1.copy(buf, 3, 3);
    UNIT_TEST(2, copyLen);
    UNIT_TEST('4', buf[0]);

    tiny_stl::string_view str4 = str1.substr(1, 4);
    UNIT_TEST(4, str4.length());
    UNIT_TEST('2', str4.front());
    str4 = str1.substr(2, 4);
    UNIT_TEST(3, str4.size());

    tiny_stl::string_view str5 = "xxxaaabcdefgxxxedf";
    tiny_stl::string_view str6 = "xxxaaabcd";
    UNIT_TEST(true, str5.compare(str6) > 0);
    UNIT_TEST(true, str5.compare(str5) == 0);
    UNIT_TEST(true, str6.compare("xxxaaabcd") == 0);
    UNIT_TEST(true, str5.compare(0, 9, str6, 0, 10) == 0);

    UNIT_TEST(true, str5.starts_with("xxx"));
    UNIT_TEST(true, str5.starts_with('x'));
    UNIT_TEST(true, str5.starts_with(str6));
    UNIT_TEST(true, str5.ends_with("xxedf"));
    UNIT_TEST(true, str5.ends_with('f'));

    UNIT_TEST(0, str5.find("xxx", 0));
    UNIT_TEST(12, str5.find("xxx", 1));
    UNIT_TEST(12, str5.rfind("xxx"));
    UNIT_TEST(12, str5.rfind("xxx", 12));
    UNIT_TEST(0, str5.rfind("xxx", 11));

#ifdef TINY_STL_CXX14
    using namespace tiny_stl::literals::string_view_literals;
    auto str7 = "xxx"_sv;
    UNIT_TEST(true,
              (tiny_stl::is_same_v<decltype(str7), tiny_stl::string_view>));
#endif // TINY_STL_CXX14
}

void testCowString() {
    using std::cout;
    using std::endl;

    tiny_stl::cow_string s0;
    UNIT_TEST(true, s0.empty());

    tiny_stl::cow_string s1 = "1234";
    UNIT_TEST(4, s1.size());
    UNIT_TEST('1', *s1.begin());

    tiny_stl::cow_string s2(s1);
    tiny_stl::cow_string s3(tiny_stl::move(s2));
    UNIT_TEST(true, s1 == s3);

    s3.front() = 'a';
    UNIT_TEST('1', *s1.begin()); // test implement of reference count
    UNIT_TEST('a', *s3.begin());

    tiny_stl::cow_string s4 = "123";
    tiny_stl::cow_string s5(s4);

    s4.resize(1);
    UNIT_TEST(1, s4.size());
    s4.resize(5);
    UNIT_TEST(5, s4.size());
    s4.resize(10, 'a');
    UNIT_TEST('1', s4.front());
    UNIT_TEST(10, s4.size());
    UNIT_TEST('a', s4.back());

    s4 = s5;
    UNIT_TEST(3, s4.size());
    tiny_stl::cow_string s6 = "abcd";
    tiny_stl::cow_string s7(s6);
    UNIT_TEST(4, s7.size());
    s4 = s6;
    UNIT_TEST(4, s4.size());
    s4 = "edcba";
    UNIT_TEST(5, s4.size());
    UNIT_TEST('e', s4.c_front()); // addition
    s4 = tiny_stl::move(s6);
    UNIT_TEST(4, s4.size());
    UNIT_TEST('a', *s4.cbegin());
    UNIT_TEST('a', s7.c_front());

    tiny_stl::cow_wstring ws1 = L"abc";
    UNIT_TEST(L'a', *ws1.cbegin());

    tiny_stl::cow_string s9 = "1223442435325324";
    tiny_stl::cow_string s8 = s9;
    s8.push_back('a');
    UNIT_TEST('a', s8.back());
    // cout << s8 << endl;
    s8.push_back('b');
    UNIT_TEST('b', s8.back());
    tiny_stl::cow_string s10 = "1234567891";
    s10.erase(2, 4);
    UNIT_TEST(6, s10.size());
    s10.erase(1, 10);
    UNIT_TEST(1, s10.size());
    tiny_stl::cow_string s11 = "123456";
    auto iter1 = s11.erase(--s11.end());
    UNIT_TEST(5, s11.size());
    UNIT_TEST('5', s11.back());
    UNIT_TEST(true, iter1 == s11.end());
    iter1 = s11.erase(s11.begin());
    UNIT_TEST(4, s11.size());
    UNIT_TEST('2', s11.front());
    UNIT_TEST('2', *iter1);

    s11 = "12345";
    iter1 = s11.erase(s11.begin() + 1, s11.begin() + 3);
    UNIT_TEST(3, s11.size());
    UNIT_TEST('4', *iter1);
    s11.pop_back();
    UNIT_TEST('4', s11.back());
    iter1 = s11.erase(s11.begin(), s11.end());
    UNIT_TEST(true, iter1 == s11.end());

    tiny_stl::cow_string s12 = "123456";
    UNIT_TEST(3, s12.find('4', 0));
    UNIT_TEST(static_cast<std::size_t>(-1), s12.find('a', 0));
    UNIT_TEST(4, s12.substr(2).size());
    UNIT_TEST(2, s12.find("3456", 2));
    UNIT_TEST(static_cast<std::size_t>(-1), s12.find("42", 0, 2));
    UNIT_TEST(3, s12.find("42", 0, 1));
    s12 = "this is a string";
    UNIT_TEST(5, s12.rfind("is", 5));
    UNIT_TEST(2, s12.rfind("is", 3));
    UNIT_TEST(10, s12.rfind('s'));
    UNIT_TEST(tiny_stl::cow_string::npos, s12.rfind("that"));

    //tiny_stl::cow_string s13 = "12345";

    // cout << (s13.replace(1, 2, 3, 'a')) << endl;

    tiny_stl::cow_string s14 = "  hello  ";
    tiny_stl::cow_string s15 = s14;
    s14.ltrim();
    UNIT_TEST(7, s14.size());
    UNIT_TEST(9, s15.size());
    UNIT_TEST(true, s14 == "hello  ");
    s14.rtrim();
    UNIT_TEST(5, s14.size());
    UNIT_TEST(true, s14 == "hello");
    s15.trim();
    UNIT_TEST(5, s15.size());
    UNIT_TEST(true, s15 == "hello");

    int x = 42;
    tiny_stl::cow_string s16 = tiny_stl::to_cow_string(x);
    UNIT_TEST(tiny_stl::cow_string{"42"}, s16);
    x = INT_MAX;
    s16 = tiny_stl::to_cow_string(x);
    UNIT_TEST(tiny_stl::cow_string{"2147483647"}, s16);
    x = INT_MIN;
    s16 = tiny_stl::to_cow_string(x);
    UNIT_TEST(tiny_stl::cow_string{"-2147483648"}, s16);

    long long llx = LLONG_MAX;
    s16 = tiny_stl::to_cow_string(llx);
    UNIT_TEST(tiny_stl::cow_string{"9223372036854775807"}, s16);

    llx = LLONG_MIN;
    s16 = tiny_stl::to_cow_string(llx);
    UNIT_TEST(tiny_stl::cow_string{"-9223372036854775808"}, s16);
}

void testString() {
    tiny_stl::string str1;
    UNIT_TEST(true, str1.empty());
    UNIT_TEST(0, str1.size());

    tiny_stl::string str2(8, 'a');
    UNIT_TEST(false, str2.empty());
    UNIT_TEST(8, str2.size());

    tiny_stl::string str3(20, 'a');
    UNIT_TEST(false, str3.empty());
    UNIT_TEST(20, str3.size());

    tiny_stl::string str4{str2};
    UNIT_TEST(8, str4.size());

    tiny_stl::string str5{str2, 3};
    UNIT_TEST(5, str5.size());

    tiny_stl::string str6{str3, 1, 17};
    UNIT_TEST(17, str6.size());

    tiny_stl::string str7 = "abcdefg";
    UNIT_TEST(7, str7.size());
    UNIT_TEST('a', str7.front());
    UNIT_TEST('g', str7.back());

    tiny_stl::string str8 = "000111222333444555666777888999";
    UNIT_TEST(30, str8.size());
    UNIT_TEST('1', str8[4]);
    UNIT_TEST('3', str8.at(10));

    str3 = str7;
    UNIT_TEST(7, str3.size());
    UNIT_TEST('a', *str3.begin());
    UNIT_TEST('b', *tiny_stl::next(str3.begin()));

    str3.push_back('h');
    UNIT_TEST(8, str3.size());
    UNIT_TEST('h', str3.back());

    str8.push_back('a');
    UNIT_TEST(31, str8.size());
    UNIT_TEST('a', str8.back());

    str3.append(3, 'x');
    UNIT_TEST(11, str3.size());
    UNIT_TEST('x', str3.back());

    str8.append(32, '!');
    UNIT_TEST(63, str8.size());
    UNIT_TEST('!', str8.back());

    str3.append("R0530");
    UNIT_TEST(16, str3.size());
    UNIT_TEST('0', str3.back());

    str3.erase(8, 3);
    UNIT_TEST(13, str3.size());
    UNIT_TEST('R', str3[8]);

    str3.erase(str3.begin());
    UNIT_TEST(12, str3.size());
    UNIT_TEST('b', str3.front());

    auto iter3 = str3.begin();
    iter3 += 7;
    str3.erase(str3.begin(), iter3);
    UNIT_TEST(5, str3.size());
    UNIT_TEST('R', str3[0]);

    tiny_stl::string str9 = "125";
    str9.insert(2, 2, '3');
    UNIT_TEST(5, str9.size());
    UNIT_TEST('3', str9[2]);
    UNIT_TEST('5', str9[4]);

    str9 = "126";
    str9.insert(2, "345");
    UNIT_TEST(6, str9.size());
    UNIT_TEST('3', str9[2]);
    UNIT_TEST('6', str9.back());

    tiny_stl::string str10 = "ceb";
    str10.insert(1, 30, 'e');
    UNIT_TEST(33, str10.size());

    tiny_stl::string str11 = "abc";
    str11.insert(1, "012345678901234567890123456789");
    UNIT_TEST(33, str11.size());
    UNIT_TEST('0', str11[1]);

    tiny_stl::string str12 = "abcgh";
    str12.insert(3, "def");
    UNIT_TEST(8, str12.size());
    UNIT_TEST('d', str12[3]);
    UNIT_TEST('e', str12[4]);
    UNIT_TEST('f', str12[5]);

    tiny_stl::string str13 = "116";
    str13.insert(1, "23456789101112131415");
    UNIT_TEST(23, str13.size());
    UNIT_TEST('2', str13[1]);

    UNIT_TEST(true, str12.compare(str13) > 0);
    UNIT_TEST(true, str3.compare("R0530") == 0);

    tiny_stl::string str14 = "abcdef";
    str14.resize(7, 'g');
    UNIT_TEST(7, str14.size());
    UNIT_TEST('g', str14.back());

    str14.resize(3);
    UNIT_TEST(3, str14.size());
    UNIT_TEST('c', str14.back());

    tiny_stl::string str15 = "This is a string";
    UNIT_TEST(2, str15.find("is"));
    UNIT_TEST(5, str15.rfind("is"));
    UNIT_TEST(3, str15.find('s'));
    UNIT_TEST(6, str15.find('s', 4));
    UNIT_TEST(13, str15.rfind('i'));
    UNIT_TEST(5, str15.rfind("is", 5));

    tiny_stl::string str16 = "1234555578";
    str16.replace(5, 3, "6");
    UNIT_TEST(8, str16.size());
    UNIT_TEST('6', str16[5]);

    tiny_stl::string str17 = "123x789";
    str17.replace(3, 1, "456");
    UNIT_TEST(9, str17.size());

    str16.replace(5, 1, 3, '5');
    UNIT_TEST(10, str16.size());
}

void testRBTree() {
    // tiny_stl::_RBTree<int, tiny_stl::less<int>, tiny_stl::allocator<int>,
    // false> tree;

    // for (int i = 5; i > 0; --i)
    /* tree._Insert_unique(0);
    tree._Insert_unique(3);
    tree._Insert_unique(1);
    tree._Insert_unique(2);
    tree._Insert_unique(2);
    tree._Insert_unique(10);
    tree._Insert_unique(3);
    tree._Insert_unique(5);*/
    // for (int i = 5; i < 10; ++i)
    //  tree.insert_equal(i);
}

void testSet() {
    tiny_stl::set<int> s = {5, 4, 3, 6, 7, 3, 5, 7, 8, 4, 6, 8};
    s.insert(1);
    UNIT_TEST(1, *s.begin());
    UNIT_TEST(7, s.size());

    auto num = s.erase(8);
    UNIT_TEST(7, *(--s.end()));
    UNIT_TEST(6, s.size());
    UNIT_TEST(1, num);
    s.erase(s.begin());
    UNIT_TEST(3, *s.begin());
    UNIT_TEST(5, s.size());
    s.emplace(2);
    UNIT_TEST(2, *s.begin());
    UNIT_TEST(6, s.size());
    auto iter = s.find(4);
    UNIT_TEST(4, *iter);

    tiny_stl::multiset<int> ms = {5, 4, 3, 6, 7, 3, 5, 7, 8, 4, 6, 8};

    ms.insert(1);
    UNIT_TEST(1, *ms.begin());
    UNIT_TEST(13, ms.size());

    num = ms.erase(8);
    UNIT_TEST(7, *(--ms.end()));
    UNIT_TEST(11, ms.size());
    UNIT_TEST(2, num);
    ms.erase(ms.begin());
    UNIT_TEST(3, *ms.begin());
    UNIT_TEST(10, ms.size());
    ms.emplace(2);
    UNIT_TEST(2, *ms.begin());
    UNIT_TEST(11, ms.size());
    auto iter1 = ms.find(4);
    UNIT_TEST(4, *iter1);

    // print_elements(ms);

    tiny_stl::multiset<int> s1;
    for (int i = 0; i < 1000; ++i) {
        s1.insert(rand());
    }
    UNIT_TEST(true, tiny_stl::is_sorted(s1.begin(), s1.end()));
    UNIT_TEST(1000, s1.size());
}

void testMap() {
    tiny_stl::map<int, double> m{{2, 2.2}, {3, 3.3}, {6, 6.6}, {4, 4.4},
                                 {3, 3.3}, {0, 0.0}, {1, 1.1}};
    auto p = m.insert({5, 5.5});
    UNIT_TEST(7, m.size());
    UNIT_TEST(true, p.second);
    p = m.insert({1, 3.4});
    UNIT_TEST(7, m.size());
    UNIT_TEST(false, p.second);
    UNIT_TEST(0, m.begin()->first);
    UNIT_TEST(0.0, m.begin()->second);
    double val = m.at(3);
    UNIT_TEST(3.3, val);
    val = m[7];
    UNIT_TEST(0.0, val);
    UNIT_TEST(8, m.size());
    m[7] = 7.7;
    UNIT_TEST(7.7, m[7]);

    // print_element_with_pair(m);
    auto m1 = m;
    UNIT_TEST(8, m1.size());
    // print_element_with_pair(m1);

    auto m2 = tiny_stl::move(m1);
    UNIT_TEST(8, m2.size());
    UNIT_TEST(0, m1.size());
    // print_element_with_pair(m2);

    tiny_stl::multimap<int, double> mm{{2, 2.2}, {3, 3.3}, {6, 6.6}, {4, 4.4},
                                       {3, 3.3}, {0, 0.0}, {1, 1.1}};

    UNIT_TEST(7, mm.size());
}

void testTuple() {
    tiny_stl::tuple<int, double, double> t{2, 3.0, 2.2};
    UNIT_TEST(2, t.get_head());
    UNIT_TEST(2, tiny_stl::get<0>(t));
    UNIT_TEST(3.0, t.get_tail().get_head());
    UNIT_TEST(3.0, tiny_stl::get<1>(t));
    UNIT_TEST(2.2, t.get_tail().get_tail().get_head());
    UNIT_TEST(2.2, tiny_stl::get<2>(t));

    tiny_stl::tuple<long, double, double> t1;
    t1 = tiny_stl::move(t);

    UNIT_TEST(2, t1.get_head());
    UNIT_TEST(2.2, t1.get_tail().get_tail().get_head());

    tiny_stl::pair<int, float> p1 = {2, 2.0f};

    tiny_stl::tuple<int, double> t2;
    t2 = p1;

    UNIT_TEST(2, t2.get_head());
    UNIT_TEST(2.0f, t2.get_tail().get_head());

    tiny_stl::tuple<int, double> t3{1, 3.0};

    tiny_stl::swap(t2, t3);
    UNIT_TEST(1, t2.get_head());
    UNIT_TEST(false, t2 == t3);
    UNIT_TEST(true, t2 < t3);
    UNIT_TEST(3, tiny_stl::tuple_size<decltype(t)>::value);
}

void testUnorderSet() {
    tiny_stl::unordered_set<int> us = {1, 2, 3, 5, 5};
    UNIT_TEST(1, *us.find(1));
    UNIT_TEST(2, *us.find(2));
    UNIT_TEST(3, *us.find(3));
    UNIT_TEST(5, *us.find(5));
    auto range = us.equal_range(5);
    UNIT_TEST(5, *range.first);
    UNIT_TEST(1, us.count(5));

    // test copy constructor
    auto us1(us);
    UNIT_TEST(1, *us1.find(1));
    UNIT_TEST(1, us1.count(5));

    // test move constructor
    auto us2 = tiny_stl::move(us1);
    UNIT_TEST(1, *us2.find(1));
    UNIT_TEST(1, us2.count(5));

    us.erase(5);
    UNIT_TEST(0, us.count(5));

    tiny_stl::unordered_set<int> us3;

    for (int i = 0; i < 1000; ++i) {
        us3.insert(rand());
    }
    us3.insert(10);
    UNIT_TEST(1, us3.count(10));

    us.swap(us3);
    UNIT_TEST(1, us.count(10));

    tiny_stl::unordered_multiset<int> ums = {1, 2, 3, 5, 5};
    UNIT_TEST(1, *ums.find(1));
    UNIT_TEST(2, *ums.find(2));
    UNIT_TEST(3, *ums.find(3));
    UNIT_TEST(5, *ums.find(5));
    auto range1 = ums.equal_range(5);
    UNIT_TEST(5, *range1.first);
    UNIT_TEST(2, ums.count(5));

    ums.erase(5);
    UNIT_TEST(0, ums.count(5));

    tiny_stl::unordered_multiset<int> ums1;

    for (int i = 0; i < 1000; ++i) {
        ums1.insert(i);
    }

    UNIT_TEST(1000, ums1.size());
    ums1.insert(10);
    UNIT_TEST(10, *ums1.find(10));

    ums.swap(ums1);
    UNIT_TEST(1001, ums.size());
}

void testUnorderedMap() {
    tiny_stl::unordered_map<int, double> um{
        {2, 2.2}, {3, 3.3}, {6, 6.6}, {4, 4.4}, {3, 3.3}, {0, 0.0}, {1, 1.1}};
    auto p = um.insert({5, 5.5});
    UNIT_TEST(7, um.size());
    UNIT_TEST(true, p.second);
    p = um.insert({1, 3.4});
    UNIT_TEST(7, um.size());
    UNIT_TEST(false, p.second);
    auto iter = um.find(0);
    UNIT_TEST(0, iter->first);
    UNIT_TEST(0.0, iter->second);
    double val = um.at(3);
    UNIT_TEST(3.3, val);

    val = um[7];
    UNIT_TEST(0.0, val);
    UNIT_TEST(8, um.size());
    um[7] = 7.7;
    UNIT_TEST(7.7, um[7]);

    // print_element_with_pair(um);
    auto um1 = um;
    UNIT_TEST(8, um1.size());
    // print_element_with_pair(um1);

    auto um2 = tiny_stl::move(um1);
    UNIT_TEST(8, um2.size());
    UNIT_TEST(0, um1.size());
    // print_element_with_pair(um2);

    tiny_stl::unordered_multimap<int, double> umm{
        {2, 2.2}, {3, 3.3}, {6, 6.6}, {4, 4.4}, {3, 3.3}, {0, 0.0}, {1, 1.1}};

    UNIT_TEST(7, umm.size());
}

void testAll() {
    testUtility();
    testTypeTraits();
    testAlgorithm();
    testArray();
    testMemory();
    testVector();
    testList();
    testForwardList();
    testDeque();
    testAdaptor();
    testCowString();
    testString();
    testStringView();
    testRBTree();
    testSet();
    testMap();
    testTuple();
    testUnorderSet();
    testUnorderedMap();
}

int main() {
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    testAll();
    std::cout << test_pass << "/" << test_count << " (passed "
              << test_pass * 100.0 / test_count << "%)" << std::endl;
    return 0;
}
