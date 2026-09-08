/*
 * The allocation functions backing libewokstl.a.
 *
 * <new> declares all of these; the placement forms are inline there and need no
 * object.  The nothrow forms are new - qtranslator.cpp does
 * `new (std::nothrow) char[d->unmapLength]`, and without them that is an
 * undefined reference at link time.
 *
 * Note on failure behaviour: operator new() returns whatever malloc() returned,
 * including NULL.  The standard requires it to throw std::bad_alloc instead,
 * but this platform builds with -fno-exceptions and has no unwinder, so there
 * is nothing to throw.  That behaviour predates this file and is deliberately
 * left alone - every existing C++ program on EwokOS already links against it.
 */

#include <new>
#include <cstdlib>

/* The tag object every nothrow new-expression names. */
const std::nothrow_t std::nothrow{};

void* operator new(std::size_t size) {
    return malloc(size);
}

void* operator new[](std::size_t size) {
    return malloc(size);
}

void operator delete(void* ptr) noexcept {
    free(ptr);
}

void operator delete[](void* ptr) noexcept {
    free(ptr);
}

// C++14 sized delete operators
void operator delete(void* ptr, std::size_t) noexcept {
    free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    free(ptr);
}

// nothrow forms
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return malloc(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return malloc(size);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    free(ptr);
}
