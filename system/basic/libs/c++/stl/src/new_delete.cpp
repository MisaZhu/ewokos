#include <stddef.h>
#include <stdlib.h>

// C++ new/delete operators for embedded systems

void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void* ptr) {
    free(ptr);
}

void operator delete[](void* ptr) {
    free(ptr);
}

// C++14 sized delete operators
void operator delete(void* ptr, size_t) {
    free(ptr);
}

void operator delete[](void* ptr, size_t) {
    free(ptr);
}
