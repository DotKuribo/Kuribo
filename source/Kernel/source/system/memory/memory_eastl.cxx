#include "system/memory.hxx"

// EASTL

void* operator new[](size_t size, const char* pName, int flags,
                     unsigned debugFlags, const char* file, int line) {
  return kuribo::mem::Alloc(size, kuribo::mem::GetDefaultHeap(), 8);
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, int flags, unsigned debugFlags,
                     const char* file, int line) {
  KURIBO_ASSERT(alignment == alignmentOffset);
  return kuribo::mem::Alloc(size, kuribo::mem::GetDefaultHeap(), alignment);
}
