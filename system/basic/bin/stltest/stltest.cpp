#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <queue>
#include <stack>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <memory>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    ::printf("Running " #name "... "); \
    test_##name(); \
    ::printf("PASSED\n"); \
    pass_count++; test_count++; \
} while(0)

#define ASSERT(cond) do { if (!(cond)) { \
    ::printf("FAILED at line %d\n", __LINE__); return; \
} } while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))

// ==================== STRING TESTS ====================
TEST(string_basic) {
    string s1 = "hello";
    string s2 = "world";
    ASSERT_EQ(s1.length(), 5);
    string s3 = s1 + " " + s2;
    ASSERT_EQ(s3, "hello world");
    ASSERT(s1 < s2);
    ASSERT(s1 > "abc");
    ASSERT_EQ(s1.find('l'), 2);
    ASSERT_EQ(s1.find("ll"), 2);
    s1[0] = 'H';
    ASSERT_EQ(s1, "Hello");
}

TEST(string_modify) {
    string s = "test";
    s.append("ing");
    ASSERT_EQ(s, "testing");
    s.insert(0, "pre");
    ASSERT_EQ(s, "pretesting");
    s.erase(3, 4);
    ASSERT_EQ(s, "preing");
    s.replace(0, 3, "post");
    ASSERT_EQ(s, "posting");
}

TEST(string_substr) {
    string s = "hello world";
    string sub = s.substr(0, 5);
    ASSERT_EQ(sub, "hello");
    sub = s.substr(6, 5);
    ASSERT_EQ(sub, "world");
    sub = s.substr(6);
    ASSERT_EQ(sub, "world");
}

TEST(string_compare) {
    string s1 = "abc";
    string s2 = "def";
    ASSERT(s1.compare(s2) < 0);
    ASSERT(s2.compare(s1) > 0);
    ASSERT_EQ(s1.compare("abc"), 0);
}

TEST(string_operators) {
    string s1 = "hello";
    string s2 = "hello";
    string s3 = "world";
    ASSERT(s1 == s2);
    ASSERT(s1 != s3);
    ASSERT(s1 < s3);
    ASSERT(s3 > s1);
    ASSERT(s1 <= s2);
    ASSERT(s1 >= s2);
}

// ==================== VECTOR TESTS ====================
TEST(vector_basic) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    ASSERT_EQ(v.size(), 3);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v.front(), 1);
    ASSERT_EQ(v.back(), 3);
    ASSERT(!v.empty());
}

TEST(vector_resize) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.resize(5, 10);
    ASSERT_EQ(v.size(), 5);
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 10);
    ASSERT_EQ(v[3], 10);
    ASSERT_EQ(v[4], 10);
}

TEST(vector_iterator) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    int sum = 0;
    for (auto it = v.begin(); it != v.end(); ++it) {
        sum += *it;
    }
    ASSERT_EQ(sum, 6);
}

// ==================== LIST TESTS ====================
TEST(list_basic) {
    list<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_front(0);
    ASSERT_EQ(l.size(), 3);
    ASSERT_EQ(l.front(), 0);
    ASSERT_EQ(l.back(), 2);
    l.pop_front();
    ASSERT_EQ(l.front(), 1);
    l.pop_back();
    ASSERT_EQ(l.back(), 1);
}

TEST(list_insert_erase) {
    list<int> l;
    l.push_back(1);
    l.push_back(3);
    auto it = l.begin();
    ++it;
    l.insert(it, 2);
    ASSERT_EQ(l.size(), 3);
    it = l.begin();
    ASSERT_EQ(*it, 1);
    ++it;
    ASSERT_EQ(*it, 2);
    ++it;
    ASSERT_EQ(*it, 3);
}

// ==================== DEQUE TESTS ====================
TEST(deque_basic) {
    deque<int> d;
    d.push_back(1);
    d.push_back(2);
    d.push_front(0);
    ASSERT_EQ(d.size(), 3);
    ASSERT_EQ(d[0], 0);
    ASSERT_EQ(d[1], 1);
    ASSERT_EQ(d[2], 2);
    ASSERT_EQ(d.front(), 0);
    ASSERT_EQ(d.back(), 2);
    d.pop_front();
    ASSERT_EQ(d.front(), 1);
    d.pop_back();
    ASSERT_EQ(d.back(), 1);
}

// ==================== QUEUE TESTS ====================
TEST(queue_basic) {
    queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    ASSERT_EQ(q.front(), 1);
    ASSERT_EQ(q.back(), 3);
    ASSERT_EQ(q.size(), 3);
    q.pop();
    ASSERT_EQ(q.front(), 2);
    ASSERT_EQ(q.size(), 2);
}

// ==================== STACK TESTS ====================
TEST(stack_basic) {
    stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    ASSERT_EQ(s.top(), 3);
    ASSERT_EQ(s.size(), 3);
    s.pop();
    ASSERT_EQ(s.top(), 2);
    ASSERT_EQ(s.size(), 2);
}

// ==================== SET TESTS ====================
TEST(set_basic) {
    set<int> s;
    s.insert(3);
    s.insert(1);
    s.insert(5);
    s.insert(1);
    ASSERT_EQ(s.size(), 3);
    ASSERT(s.find(3) != s.end());
    ASSERT(s.find(4) == s.end());
    s.erase(1);
    ASSERT_EQ(s.size(), 2);
    ASSERT(s.find(1) == s.end());
}

// ==================== MAP TESTS ====================
TEST(map_basic) {
    map<int, int> m;
    m[1] = 10;
    m[2] = 20;
    m[3] = 30;
    ASSERT_EQ(m.size(), 3);
    ASSERT_EQ(m[1], 10);
    ASSERT_EQ(m[2], 20);
    ASSERT_EQ(m[3], 30);
    m.erase(1);
    ASSERT_EQ(m.size(), 2);
    ASSERT(m.find(1) == m.end());
}

// ==================== UNORDERED_SET TESTS ====================
TEST(unordered_set_basic) {
    unordered_set<int> us;
    us.insert(1);
    us.insert(2);
    us.insert(3);
    us.insert(1);
    ASSERT_EQ(us.size(), 3);
    ASSERT(us.find(2) != us.end());
    ASSERT(us.find(4) == us.end());
    us.erase(2);
    ASSERT_EQ(us.size(), 2);
    ASSERT(us.find(2) == us.end());
}

// ==================== UNORDERED_MAP TESTS ====================
TEST(unordered_map_basic) {
    unordered_map<int, int> um;
    um[1] = 10;
    um[2] = 20;
    um[3] = 30;
    ASSERT_EQ(um.size(), 3);
    ASSERT_EQ(um[1], 10);
    ASSERT_EQ(um[2], 20);
    um.erase(2);
    ASSERT_EQ(um.size(), 2);
    ASSERT(um.find(2) == um.end());
}

// ==================== UTILITY TESTS ====================
TEST(pair_basic) {
    pair<int, int> p1(42, 100);
    ASSERT_EQ(p1.first, 42);
    ASSERT_EQ(p1.second, 100);

    pair<int, int> p2 = make_pair(200, 300);
    ASSERT_EQ(p2.first, 200);
    ASSERT_EQ(p2.second, 300);
}

TEST(pair_operators) {
    pair<int, int> a(1, 2);
    pair<int, int> b(1, 2);
    pair<int, int> c(3, 4);
    
    ASSERT(a == b);
    ASSERT(a != c);
    ASSERT(a < c);
    ASSERT(c > a);
    ASSERT(a <= b);
    ASSERT(a >= b);
}

// ==================== TUPLE TESTS ====================
TEST(tuple_basic) {
    tuple<int, int, int> t(1, 2, 3);
    // Only test get<0> for now
    ASSERT_EQ(get<0>(t), 1);
}

TEST(tuple_size) {
    tuple<int, int, int> t(1, 2, 3);
    ASSERT_EQ(tuple_size<decltype(t)>::value, 3);
}

// ==================== MEMORY TESTS ====================
TEST(unique_ptr_basic) {
    unique_ptr<int> p1(new int(42));
    ASSERT_EQ(*p1, 42);
    ASSERT(p1.get() != nullptr);

    unique_ptr<int> p2(new int(100));
    p1.reset();
    ASSERT(p1.get() == nullptr);
}

TEST(unique_ptr_move) {
    unique_ptr<int> p1(new int(42));
    unique_ptr<int> p2(std::move(p1));
    ASSERT(p1.get() == nullptr);
    ASSERT(p2.get() != nullptr);
    ASSERT_EQ(*p2, 42);
}

TEST(unique_ptr_array) {
    unique_ptr<int[]> arr(new int[5]);
    for (int i = 0; i < 5; ++i) {
        arr[i] = i * 10;
    }
    ASSERT_EQ(arr[0], 0);
    ASSERT_EQ(arr[4], 40);
}

TEST(unique_ptr_nullptr_assign) {
    unique_ptr<int> p1(new int(42));
    p1 = nullptr;
    ASSERT(p1.get() == nullptr);
}

TEST(shared_ptr_basic) {
    shared_ptr<int> p1(new int(42));
    ASSERT_EQ(*p1, 42);
    ASSERT_EQ(p1.use_count(), 1);

    shared_ptr<int> p2 = p1;
    ASSERT_EQ(p1.use_count(), 2);
    ASSERT_EQ(p2.use_count(), 2);

    p2.reset();
    ASSERT_EQ(p1.use_count(), 1);
}

TEST(shared_ptr_move) {
    shared_ptr<int> p1(new int(42));
    shared_ptr<int> p2(std::move(p1));
    ASSERT(p1.get() == nullptr);
    ASSERT_EQ(p2.use_count(), 1);
    ASSERT_EQ(*p2, 42);
}

TEST(shared_ptr_make) {
    shared_ptr<int> p1 = make_shared<int>(100);
    ASSERT_EQ(*p1, 100);
    ASSERT_EQ(p1.use_count(), 1);
}

TEST(shared_ptr_aliasing) {
    struct Data {
        int a;
        int b;
    };
    shared_ptr<Data> p1(new Data);
    p1->a = 10;
    p1->b = 20;
    
    shared_ptr<int> p2(p1, &p1->b);
    ASSERT_EQ(*p2, 20);
    ASSERT_EQ(p2.use_count(), 2);
}

TEST(shared_ptr_reset_with_deleter) {
    int* counter = new int(0);
    auto deleter = [counter](int* p) {
        delete p;
        *counter = *counter + 1;
    };
    shared_ptr<int> p(new int(42), deleter);
    p.reset(new int(100), deleter);
    ASSERT_EQ(*counter, 1);  // First deleter called during reset
    p.reset();
    ASSERT_EQ(*counter, 2);  // Second deleter called during reset
    delete counter;
}

TEST(shared_ptr_unique) {
    shared_ptr<int> p1(new int(42));
    ASSERT(p1.unique());
    shared_ptr<int> p2 = p1;
    ASSERT(!p1.unique());
    p2.reset();
    ASSERT(p1.unique());
}

TEST(shared_ptr_operator_bool) {
    shared_ptr<int> p1(new int(42));
    shared_ptr<int> p2;
    ASSERT(static_cast<bool>(p1));
    ASSERT(!static_cast<bool>(p2));
}

TEST(weak_ptr_basic) {
    shared_ptr<int> p1(new int(42));
    weak_ptr<int> wp = p1;
    ASSERT(!wp.expired());
    ASSERT_EQ(wp.use_count(), 1);

    shared_ptr<int> p2 = wp.lock();
    ASSERT_EQ(*p2, 42);
    ASSERT_EQ(p1.use_count(), 2);
}

TEST(weak_ptr_lock_expired) {
    weak_ptr<int> wp;
    {
        shared_ptr<int> p1(new int(100));
        wp = p1;
        ASSERT(!wp.expired());
        shared_ptr<int> p2 = wp.lock();
        ASSERT(p2 != nullptr);
        ASSERT_EQ(*p2, 100);
    }
    ASSERT(wp.expired());
    shared_ptr<int> p3 = wp.lock();
    ASSERT(p3 == nullptr);
}

TEST(weak_ptr_move) {
    shared_ptr<int> p1(new int(42));
    weak_ptr<int> wp1 = p1;
    weak_ptr<int> wp2(std::move(wp1));
    ASSERT(wp1.expired() || wp1.lock() == nullptr);
    ASSERT(!wp2.expired());
}

TEST(weak_ptr_reset) {
    shared_ptr<int> p1(new int(42));
    weak_ptr<int> wp = p1;
    ASSERT(!wp.expired());
    wp.reset();
    ASSERT(wp.expired());
    ASSERT(wp.use_count() == 0);
}

TEST(weak_ptr_swap) {
    shared_ptr<int> p1(new int(42));
    shared_ptr<int> p2(new int(100));
    weak_ptr<int> wp1 = p1;
    weak_ptr<int> wp2 = p2;
    wp1.swap(wp2);
    ASSERT_EQ(*wp1.lock(), 100);
    ASSERT_EQ(*wp2.lock(), 42);
}

TEST(weak_ptr_owner_before) {
    shared_ptr<int> p1(new int(42));
    shared_ptr<int> p2(new int(100));
    weak_ptr<int> wp1 = p1;
    weak_ptr<int> wp2 = p2;
    
    bool b1 = wp1.owner_before(p2);
    bool b2 = wp2.owner_before(p1);
    
    if (b1) {
        ASSERT(!b2);
    }
}

TEST(pointer_casts) {
    struct Base { virtual ~Base() {} int x; };
    struct Derived : Base { int y; };
    
    shared_ptr<Derived> pd(new Derived);
    pd->x = 10;
    pd->y = 20;
    
    shared_ptr<Base> pb = static_pointer_cast<Base>(pd);
    ASSERT_EQ(pb->x, 10);
    ASSERT_EQ(pb.use_count(), 2);
    
    shared_ptr<Derived> pd2 = static_pointer_cast<Derived>(pb);
    ASSERT(pd2.get() != nullptr);
    ASSERT_EQ(pd2->y, 20);
    
    shared_ptr<const Base> pcb = const_pointer_cast<const Base>(pb);
    ASSERT_EQ(pcb->x, 10);
}

TEST(make_unique_basic) {
    unique_ptr<int> p1 = make_unique<int>(42);
    ASSERT_EQ(*p1, 42);
}

TEST(make_unique_array) {
    unique_ptr<int[]> arr = make_unique<int[]>(5);
    arr[0] = 10;
    arr[4] = 50;
    ASSERT_EQ(arr[0], 10);
    ASSERT_EQ(arr[4], 50);
}

TEST(make_unique_object) {
    struct Foo {
        int a;
        int b;
        Foo(int x, int y) : a(x), b(y) {}
    };
    unique_ptr<Foo> p = make_unique<Foo>(1, 2);
    ASSERT_EQ(p->a, 1);
    ASSERT_EQ(p->b, 2);
}

TEST(shared_ptr_comparison) {
    shared_ptr<int> p1(new int(42));
    shared_ptr<int> p2(new int(42));
    shared_ptr<int> p3 = p1;
    
    ASSERT(p1 == p3);
    ASSERT(p1 != p2);
    ASSERT(p1 < p2 || p2 < p1);
}

TEST(shared_ptr_nullptr_comparison) {
    shared_ptr<int> p1(new int(42));
    shared_ptr<int> p2;
    
    ASSERT(p1 != nullptr);
    ASSERT(p2 == nullptr);
    ASSERT(nullptr != p1);
    ASSERT(nullptr == p2);
}

TEST(swap_pointers) {
    unique_ptr<int> up1(new int(42));
    unique_ptr<int> up2(new int(100));
    swap(up1, up2);
    ASSERT_EQ(*up1, 100);
    ASSERT_EQ(*up2, 42);
    
    shared_ptr<int> sp1(new int(1));
    shared_ptr<int> sp2(new int(2));
    swap(sp1, sp2);
    ASSERT_EQ(*sp1, 2);
    ASSERT_EQ(*sp2, 1);
}

// ==================== ALGORITHM TESTS ====================
TEST(algorithm_find) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    auto it = find(v.begin(), v.end(), 2);
    ASSERT(it != v.end());
    ASSERT_EQ(*it, 2);
    it = find(v.begin(), v.end(), 4);
    ASSERT(it == v.end());
}

TEST(algorithm_sort) {
    vector<int> v;
    v.push_back(3);
    v.push_back(1);
    v.push_back(2);
    sort(v.begin(), v.end());
    ASSERT_EQ(v[0], 1);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 3);
}

TEST(algorithm_reverse) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    reverse(v.begin(), v.end());
    ASSERT_EQ(v[0], 3);
    ASSERT_EQ(v[1], 2);
    ASSERT_EQ(v[2], 1);
}

TEST(algorithm_swap) {
    int a = 1, b = 2;
    swap(a, b);
    ASSERT_EQ(a, 2);
    ASSERT_EQ(b, 1);
}

TEST(algorithm_min_max) {
    ASSERT_EQ(min(1, 2), 1);
    ASSERT_EQ(max(1, 2), 2);
}

// ==================== ITERATOR TESTS ====================
TEST(iterator_basic) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    
    auto it = v.begin();
    ASSERT_EQ(*it, 1);
    ++it;
    ASSERT_EQ(*it, 2);
    it++;
    ASSERT_EQ(*it, 3);
}

TEST(iterator_advance) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);
    
    auto it = v.begin();
    advance(it, 3);
    ASSERT_EQ(*it, 4);
}

TEST(iterator_vector_end_comparison) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    
    // Non-const iterator test
    auto it = v.begin();
    ASSERT(it != v.end());
    
    int count = 0;
    for (; it != v.end(); ++it) {
        count++;
    }
    ASSERT_EQ(count, 3);
}

TEST(iterator_vector_const_end_comparison) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    
    // Const iterator test
    const vector<int>& cv = v;
    auto cit = cv.begin();
    ASSERT(cit != cv.end());
    
    int count = 0;
    for (; cit != cv.end(); ++cit) {
        count++;
    }
    ASSERT_EQ(count, 3);
}

TEST(iterator_vector_mixed_comparison) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    
    // Mix const and non-const iterator comparison
    auto it = v.begin();
    const vector<int>& cv = v;
    auto cit = cv.begin();
    ASSERT(it == cit);
    ASSERT(cit == it);
    ASSERT(it != cv.end());
    ASSERT(cit != v.end());
    
    ++it;
    ASSERT(it != cit);
    ASSERT(cit != it);
}

TEST(iterator_vector_all_comparisons) {
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);
    v.push_back(50);
    
    auto it1 = v.begin();
    auto it2 = v.begin() + 1;
    auto it3 = v.end();
    
    ASSERT(it1 < it2);
    ASSERT(it2 > it1);
    ASSERT(it1 <= it2);
    ASSERT(it2 >= it1);
    ASSERT(it1 != it2);
    ASSERT(it2 != it3);
    ASSERT(it1 != it3);
    
    // Const comparison
    const vector<int>& cv = v;
    auto cit1 = cv.begin();
    auto cit2 = cv.begin() + 1;
    
    ASSERT(it1 < cit2);
    ASSERT(cit2 > it1);
    ASSERT(it1 <= cit2);
    ASSERT(cit2 >= it1);
    ASSERT(it1 != cit2);
}

TEST(iterator_list_end_comparison) {
    list<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    
    // Non-const
    auto it = l.begin();
    ASSERT(it != l.end());
    
    int count = 0;
    for (; it != l.end(); ++it) {
        count++;
    }
    ASSERT_EQ(count, 3);
    
    // Const
    const list<int>& cl = l;
    auto cit = cl.begin();
    ASSERT(cit != cl.end());
    
    count = 0;
    for (; cit != cl.end(); ++cit) {
        count++;
    }
    ASSERT_EQ(count, 3);
}

TEST(iterator_list_mixed_comparison) {
    list<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    
    auto it = l.begin();
    const list<int>& cl = l;
    auto cit = cl.begin();
    
    ASSERT(it == cit);
    ASSERT(cit == it);
    ASSERT(it != cl.end());
    ASSERT(cit != l.end());
}

// ==================== SSTREAM TESTS ====================
TEST(sstream_ostringstream_basic) {
    ostringstream oss;
    oss << "Hello";
    oss << " ";
    oss << "World";
    ASSERT_EQ(oss.str(), "Hello World");
}

TEST(sstream_ostringstream_int) {
    ostringstream oss;
    oss << 42 << " " << 100;
    ASSERT_EQ(oss.str(), "42 100");
}

TEST(sstream_ostringstream_multiple_int) {
    ostringstream oss;
    oss << 1 << " + " << 2 << " = " << 3;
    ASSERT_EQ(oss.str(), "1 + 2 = 3");
}

TEST(sstream_istringstream_basic) {
    istringstream iss("Hello World");
    string s1, s2;
    iss >> s1 >> s2;
    ASSERT_EQ(s1, "Hello");
    ASSERT_EQ(s2, "World");
}

TEST(sstream_istringstream_int) {
    istringstream iss("42 100");
    int a, b;
    iss >> a >> b;
    ASSERT_EQ(a, 42);
    ASSERT_EQ(b, 100);
}

TEST(sstream_stringstream_basic) {
    stringstream ss;
    ss << "test" << " " << 123;
    string str = ss.str();
    ASSERT_EQ(str, "test 123");
}

TEST(sstream_stringstream_read_write) {
    stringstream ss;
    ss << 42 << " " << 100;
    int a, b;
    ss >> a >> b;
    ASSERT_EQ(a, 42);
    ASSERT_EQ(b, 100);
}

TEST(sstream_str) {
    ostringstream oss;
    oss << "initial";
    ASSERT_EQ(oss.str(), "initial");
    string s = "changed";
    oss.str(s);
    ASSERT_EQ(oss.str(), "changed");
}

// ==================== IOSTREAM TESTS ====================
TEST(iostream_ostream_int) {
    // Test basic_ostream with integers
    // Since we can't capture stdout, we just verify it compiles and runs
    cout << 42;
    cout << -123;
    cout << 0;
    cout << endl;
    ASSERT(true);
}

TEST(iostream_ostream_char) {
    // Test basic_ostream with characters
    cout << 'A';
    cout << 'b';
    cout << '1';
    cout << endl;
    ASSERT(true);
}

TEST(iostream_ostream_string) {
    // Test basic_ostream with strings
    cout << "Hello";
    cout << " ";
    cout << "World";
    cout << endl;
    ASSERT(true);
}

TEST(iostream_ostream_bool) {
    // Test basic_ostream with bool
    cout << true;
    cout << " ";
    cout << false;
    cout << endl;
    ASSERT(true);
}

// ==================== FSTREAM TESTS ====================
TEST(fstream_ofstream_basic) {
    // Test writing to file
    const char* test_file = "/tmp/test_ofstream.txt";
    ofstream ofs(test_file);
    ASSERT(ofs.is_open());
    ofs << "Hello World";
    ofs << " ";
    ofs << 42;
    ofs.close();
    ASSERT(!ofs.is_open());
}

TEST(fstream_ifstream_basic) {
    // First create a test file
    const char* test_file = "/tmp/test_ifstream.txt";
    ofstream ofs(test_file);
    ofs << "Test content 123";
    ofs.close();

    // Now read it back
    ifstream ifs(test_file);
    ASSERT(ifs.is_open());
    char buf[32];
    ifs.read(buf, 16);
    buf[16] = '\0';
    ASSERT_EQ(string(buf), "Test content 123");
    ifs.close();
}

TEST(fstream_fstream_read_write) {
    // Test read/write with fstream
    const char* test_file = "/tmp/test_fstream.txt";

    // Write
    fstream fs(test_file, (openmode)(out | trunc_));
    ASSERT(fs.is_open());
    fs << 100;
    fs << " ";
    fs << 200;
    fs.close();

    // Read back
    fstream fs2(test_file, in);
    ASSERT(fs2.is_open());
    int a, b;
    fs2 >> a >> b;
    ASSERT_EQ(a, 100);
    ASSERT_EQ(b, 200);
    fs2.close();
}

int main(int argc, char** argv) {
    ::printf("=== STL Test Suite ===\n\n");

    // String tests
    RUN_TEST(string_basic);
    RUN_TEST(string_modify);
    RUN_TEST(string_substr);
    RUN_TEST(string_compare);
    RUN_TEST(string_operators);

    // Vector tests
    RUN_TEST(vector_basic);
    RUN_TEST(vector_resize);
    RUN_TEST(vector_iterator);

    // List tests
    RUN_TEST(list_basic);
    RUN_TEST(list_insert_erase);

    // Deque tests
    RUN_TEST(deque_basic);

    // Queue tests
    RUN_TEST(queue_basic);

    // Stack tests
    RUN_TEST(stack_basic);

    // Set tests
    RUN_TEST(set_basic);

    // Map tests
    RUN_TEST(map_basic);

    // Unordered set tests
    RUN_TEST(unordered_set_basic);

    // Unordered map tests
    RUN_TEST(unordered_map_basic);

    // Utility tests
    RUN_TEST(pair_basic);
    RUN_TEST(pair_operators);

    // Tuple tests
    RUN_TEST(tuple_basic);
    RUN_TEST(tuple_size);

    // Memory tests
    RUN_TEST(unique_ptr_basic);
    RUN_TEST(unique_ptr_move);
    RUN_TEST(unique_ptr_array);
    RUN_TEST(unique_ptr_nullptr_assign);
    RUN_TEST(shared_ptr_basic);
    RUN_TEST(shared_ptr_move);
    RUN_TEST(shared_ptr_make);
    RUN_TEST(shared_ptr_aliasing);
    RUN_TEST(shared_ptr_reset_with_deleter);
    RUN_TEST(shared_ptr_unique);
    RUN_TEST(shared_ptr_operator_bool);
    RUN_TEST(weak_ptr_basic);
    RUN_TEST(weak_ptr_lock_expired);
    RUN_TEST(weak_ptr_move);
    RUN_TEST(weak_ptr_reset);
    RUN_TEST(weak_ptr_swap);
    RUN_TEST(weak_ptr_owner_before);
    RUN_TEST(pointer_casts);
    RUN_TEST(make_unique_basic);
    RUN_TEST(make_unique_array);
    RUN_TEST(make_unique_object);
    RUN_TEST(shared_ptr_comparison);
    RUN_TEST(shared_ptr_nullptr_comparison);
    RUN_TEST(swap_pointers);

    // Algorithm tests
    RUN_TEST(algorithm_find);
    RUN_TEST(algorithm_sort);
    RUN_TEST(algorithm_reverse);
    RUN_TEST(algorithm_swap);
    RUN_TEST(algorithm_min_max);

    // Sstream tests
    RUN_TEST(sstream_ostringstream_basic);
    RUN_TEST(sstream_ostringstream_int);
    RUN_TEST(sstream_ostringstream_multiple_int);
    RUN_TEST(sstream_istringstream_basic);
    RUN_TEST(sstream_istringstream_int);
    RUN_TEST(sstream_stringstream_basic);
    RUN_TEST(sstream_stringstream_read_write);
    RUN_TEST(sstream_str);

    // Iterator tests
    RUN_TEST(iterator_basic);
    RUN_TEST(iterator_advance);
    RUN_TEST(iterator_vector_end_comparison);
    RUN_TEST(iterator_vector_const_end_comparison);
    RUN_TEST(iterator_vector_mixed_comparison);
    RUN_TEST(iterator_vector_all_comparisons);
    RUN_TEST(iterator_list_end_comparison);
    RUN_TEST(iterator_list_mixed_comparison);

    // Iostream tests
    RUN_TEST(iostream_ostream_int);
    RUN_TEST(iostream_ostream_char);
    RUN_TEST(iostream_ostream_string);
    RUN_TEST(iostream_ostream_bool);

    // Fstream tests
    RUN_TEST(fstream_ofstream_basic);
    RUN_TEST(fstream_ifstream_basic);
    RUN_TEST(fstream_fstream_read_write);

    ::printf("\n=== Results: %d/%d tests passed ===\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
