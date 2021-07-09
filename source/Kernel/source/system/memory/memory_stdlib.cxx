#include "system/memory.hxx"

// C

void* _malloc(u32 s) {
  return kuribo::mem::Alloc(s, kuribo::mem::GetDefaultHeap());
}
void* _malloc_ex(u32 s, u32 a) {
  return kuribo::mem::Alloc(s, kuribo::mem::GetDefaultHeap(), a);
}

void _free(void* p) {
  if (p)
    kuribo::mem::Free(p);
}