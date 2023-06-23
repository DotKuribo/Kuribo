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
  constexpr int size = 0x100000;
  static char GLOBAL_HEAP[size];

  static kuribo::mem::FreeListHeap modules_allocator(&GLOBAL_HEAP[0], size);
  return &modules_allocator;
}

#else

//
// Configuration for PAL MKW
//

/* Defines the function addresses in NTSC-U SMS */
#define OSReport 0x80344644
#define OSPanic 0x803446C4
#define DVDReadPrio 0x8034BD74
#define DVDClose 0x8034B9DC

/* Sets GC/Wii OS functions. */
KURIBO_SET_OS(OSReport, OSFatal)

/* Sets functions to read from the disc. */
KURIBO_SET_DVD(DVDReadPrio, DVDClose)

class SMS_NTSC_U_Allocator : private kuribo::mem::Heap {
public:
  SMS_NTSC_U_Allocator(void* heap) : mHeap(heap) {}

  kuribo::mem::Heap* get() { return this; }

private:
  void* alloc(u32 size, u32 align) noexcept override {
    return ((char* (*)(unsigned, int, void*))0x802C3740)(
        size, align,
        mHeap); // JKRHeap::alloc(size_t size, int alignment, JKRHeap *heap)
  }
  void free(void* ptr) noexcept override {
    ((void (*)(void*, void*))0x802C37B8)(
        ptr, mHeap); // JKRHeap::free(void *ptr, JKRHeap *heap)
  }

  void* mHeap;
};

kuribo::DeferredInitialization<SMS_NTSC_U_Allocator> sms_allocator;
kuribo::mem::Heap* HostGetModuleAllocator() {
  KURIBO_PRINTF("Using new alloc system\n");
  sms_allocator.initialize(
      *(void**)0x8040E298); // Initialize using global pointer to root heap
  return sms_allocator->get();
}
#endif