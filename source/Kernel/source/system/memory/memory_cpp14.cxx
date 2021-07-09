#include "system/memory.hxx"

// C++14

#if __cplusplus >= 201402L || defined(_WIN32)
void operator delete(void* ptr, size_t) { operator delete(ptr); }
void operator delete[](void* ptr, size_t) { operator delete(ptr); }
#endif