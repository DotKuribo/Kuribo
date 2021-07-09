#include "system/memory.hxx"

// C++

void* operator new(size_t size) {
  return kuribo::mem::Alloc(size, kuribo::mem::GetDefaultHeap());
}
void* operator new[](size_t size) { return operator new(size); }

void operator delete(void* p) {
  if (p)
    kuribo::mem::Free(p);
}
void operator delete[](void* p) {
  if (p)
    kuribo::mem::Free(p);
}