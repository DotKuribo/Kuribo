#include "system/memory.hxx"
#include "Common/debug.h"
#include "config.h"
#include "util/deferred.hxx"

#include "FallbackAllocator/free_list_heap.hxx"

namespace kuribo {

namespace mem {
void* Alloc(u32 size, Heap& heap, u32 align) noexcept {
  return heap.alloc(size > 8 ? size : 8, align);
}

void Free(void* ptr, Heap& heap) { heap.free(ptr); }
void Free(void* ptr) {
  if (ptr == nullptr)
    return;

  KURIBO_PRINTF("Unsafe free\n");
  // TODO: Unsafe if given invalid pointer
  using alloc_header = xalloc::FreeListAllocator::AllocationHeader;
  alloc_header* pHeader = reinterpret_cast<alloc_header*>(ptr) - 1;
  pHeader->heap->Free(ptr);
}

Heap* sMem1Heap = nullptr;
Heap* sMem2Heap = nullptr;

// For allocating heaps themselves
char sSystemBuffer[1024];
DeferredInitialization<FreeListHeap> sSystemAllocator;

static void InitSystemAllocator() {
  sSystemAllocator.initialize(sSystemBuffer, sizeof(sSystemBuffer));
}

void Init(char* mem1b, u32 mem1s) {
  InitSystemAllocator();

  sMem1Heap = new (&sSystemAllocator) FreeListHeap(mem1b, mem1s);
  // sMem2Heap = new (&sSystemAllocator) FreeListHeap(mem2b, mem2s);
}

Heap& GetHeap(GlobalHeapType type) {
  switch (type) {
  case GlobalHeapType::Default:
  case GlobalHeapType::MEM1:
    return *sMem1Heap;
    // case GlobalHeapType::MEM2:
    //  return sMem2Heap;
  }
}

} // namespace mem

} // namespace kuribo