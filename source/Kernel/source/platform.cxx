#include "api/HostInterop.h"
#include "common.h"

/* Defines the function addresses in PAL MKW */
#define OSReport 0x801A25D0
#define OSFatal 0x801A4EC4
#define DVDReadPrio 0x8015E834
#define DVDClose 0x8015E568

/* Sets GC/Wii OS functions. */
KURIBO_SET_OS(OSReport, OSFatal)

/* Sets functions to read from the disc. */
KURIBO_SET_DVD(DVDReadPrio, DVDClose)

// For internal systems that should be kept separate from the rest of the game
Arena HostGetSystemArena() {
  constexpr unsigned size = float(923'448) * .7f;

  void* our_heap = *(void**)(0x802A40A4);
  char* base_addr =
      ((char* (*)(unsigned, void*, unsigned))0x80229DE0)(size, our_heap, 32);

#ifdef KURIBO_MEM_DEBUG
  KURIBO_PRINTF("ALLOCATED BLOCK AT: %p\n", base_addr);
#endif

  return Arena{.base_address = base_addr, .size = size};
}
