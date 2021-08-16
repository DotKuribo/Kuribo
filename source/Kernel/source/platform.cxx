#include "api/HostInterop.h"
#include "common.h"

#include "Common/util/deferred.hxx"
#include "FallbackAllocator/free_list_heap.hxx"

#ifdef _WIN32

#include <cstdarg>
#include <cstdio>

//
// Configuration for the windows emulator
//

void Win32_OSReport(const char* msg, ...) {
  va_list vargs;
  va_start(vargs, msg);
  vprintf(msg, vargs);
  va_end(vargs);
}

void Win32_OSFatal(u32* fg, u32* bg, const char* msg) {
  (void)fg;
  (void)bg;

  printf("[OSFatal] %s\n", msg);
  __debugbreak();
}

int Win32_DVDUnsupported(...) { return 0; }

#define OSReport (u32)(void*) Win32_OSReport
#define OSFatal (u32)(void*) Win32_OSFatal
#define DVDReadPrio (u32)(void*) Win32_DVDUnsupported
#define DVDClose (u32)(void*) Win32_DVDUnsupported

/* Sets GC/Wii OS functions. */
KURIBO_SET_OS(OSReport, OSFatal)

/* Sets functions to read from the disc. */
KURIBO_SET_DVD(DVDReadPrio, DVDClose)

kuribo::mem::Heap* HostGetModuleAllocator() {
  constexpr int size = float(923'448) * .7f;
  static char GLOBAL_HEAP[size];

  static kuribo::mem::FreeListHeap modules_allocator(&GLOBAL_HEAP[0], size);
  return &modules_allocator;
}

#else

//
// Configuration for PAL MKW
//

/* Defines the function addresses in PAL MKW */
#define OSReport 0x801A25D0
#define OSFatal 0x801A4EC4
#define DVDReadPrio 0x8015E834
#define DVDClose 0x8015E568

/* Sets GC/Wii OS functions. */
KURIBO_SET_OS(OSReport, OSFatal)

/* Sets functions to read from the disc. */
KURIBO_SET_DVD(DVDReadPrio, DVDClose)

class MKW_PAL_Allocator : private kuribo::mem::Heap {
public:
  MKW_PAL_Allocator(void* heap) : mHeap(heap) {}

  kuribo::mem::Heap* get() { return this; }

private:
  void* alloc(u32 size, u32 align) noexcept override {
    return ((char* (*)(unsigned, void*, unsigned))0x80229DE0)(size, mHeap,
                                                              align);
  }
  void free(void* ptr) noexcept override {
    (*(void (**)(void*, void*))(*(s32*)mHeap + 24))(mHeap, ptr);
  }

  void* mHeap;
};

kuribo::DeferredInitialization<MKW_PAL_Allocator> mkw_allocator;
kuribo::mem::Heap* HostGetModuleAllocator() {
  KURIBO_PRINTF("Using new alloc system\n");
  mkw_allocator.initialize(*(void**)0x802A40A4);
  return mkw_allocator->get();
}
#endif