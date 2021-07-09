#include "system/memory.hxx"
#include "Common/debug.h"
#include "config.h"
#include "util/deferred.hxx"

#include "FallbackAllocator/free_list_heap.hxx"

namespace kuribo {
namespace mem {

Heap* sMem1Heap = nullptr;

void* Alloc(u32 size, Heap& heap, u32 align) noexcept {
  if (void* buf = heap.alloc(size > 8 ? size : 8, align); buf != nullptr) {
    return buf;
  }

  KURIBO_LOG("Failed to allocate\n");
  return nullptr;
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


char sSystemBuffer[1024];
DeferredInitialization<FreeListHeap> sSystemAllocator;


void Init() {
  sSystemAllocator.initialize(sSystemBuffer, sizeof(sSystemBuffer));

  sMem1Heap = &static_cast<FreeListHeap&>(sSystemAllocator);
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