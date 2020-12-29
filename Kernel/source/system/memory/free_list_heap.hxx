#pragma once

#include "xalloc/FreeListAllocator.h"
#include <memory/heap.hxx>

namespace kuribo::mem {

struct FreeListHeap : public Heap {
  FreeListHeap(char* data, u32 capacity)
      : mAlloc(*data, capacity,
               xalloc::FreeListAllocator::PlacementPolicy::FIND_FIRST) {}
  xalloc::FreeListAllocator& getAlloc() { return mAlloc; }
  void* alloc(u32 size, u32 align = 8) noexcept override final {
    return getAlloc().Allocate(size, align > 8 ? align : 8);
  }
  void free(void* p) noexcept override final { getAlloc().Free(p); }
  xalloc::FreeListAllocator mAlloc;
};

} // namespace kuribo::mem