
#include "FallbackAllocator/free_list_heap.hxx"
#include "kuribo_sdk.h"

#include "engine/compiler.hpp"

KURIBO_MODULE_BEGIN("GeckoJIT", "Kuribo", "1.0") {
  KURIBO_EXPORT_AS(kuribo::kxGeckoJitCompileCodes, "kxGeckoJitCompileCodes");
}
KURIBO_MODULE_END()


// We can rely on static ctors here
char sGeckoJITBuf[2048];
kuribo::mem::FreeListHeap sGeckoJITHeap(sGeckoJITBuf, sizeof(sGeckoJITBuf));

void* AllocHack(u32 size, s32 align = 32) {
  return sGeckoJITHeap.alloc(size, align);
}
void FreeHack(void* buf) { sGeckoJITHeap.free(buf); }

void* operator new(size_t size) { return AllocHack(size); }
void* operator new[](size_t size) { return AllocHack(size); }

// EASTL

void* operator new[](size_t size, const char* pName, int flags,
                     unsigned debugFlags, const char* file, int line) {
  return AllocHack(size);
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, int flags, unsigned debugFlags,
                     const char* file, int line) {
  KURIBO_ASSERT(alignment == alignmentOffset);
  return AllocHack(size, alignment);
}
void operator delete(void* buf) { FreeHack(buf); }
void operator delete(void* buf, size_t) { FreeHack(buf); }
void operator delete[](void* buf) { FreeHack(buf); }
void operator delete[](void* buf, size_t) { FreeHack(buf); }

// We can use static ctors here
const uint32_t kuribo_ffi_os_report =
    (uint32_t)KURIBO_GET_PROCEDURE("OSReport");

// TODO: Hack
extern "C"  void __cxa_atexit() {}
#include "../source/Kernel/source/vendor/libc.c"
