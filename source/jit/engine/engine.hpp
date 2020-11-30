#pragma once

#include "common.h"
#include <jit/allocator/frame.hpp>

namespace gecko_jit {

class JITEngine {
public:
  JITEngine(u8* memory_begin, size_t memory_size)
      : mCodeAllocator(memory_begin, memory_size) {}

  enum class CodeHeap {
    //! Large function frequently expanded with 02, 04, etc codes
    ExecuteEveryFrame,

    //! Data, function bodies hooked into existing code (C0).
    ExecuteOnDemand
  };

  //! @brief Allocate a block on each code heap.
  //!
  //! It's important to note that ExecuteEveryFrame allocations are purely
  //! contiguous. Allocating four bytes at a time behaves as allocating one
  //! eight byte chunk.
  u8* alloc(CodeHeap heap, size_t size) {
    switch (heap) {
    case CodeHeap::ExecuteEveryFrame:
      return mCodeAllocator.alloc(size, true);
    case CodeHeap::ExecuteOnDemand:
      return mCodeAllocator.alloc(size, false);
    default:
      return nullptr;
    }
  }

  // Backs up the allocation position.
  // -1 if failed
  u32 getSaveState() const { return mCodeAllocator.getSaveState(); }

  // Restore allocation position. Does not zerofill used memory.
  // return if success
  bool applySaveState(u32 save) { return mCodeAllocator.applySaveState(save); }

  FrameAllocator::MemoryRegion computeRemaining() const {
    return mCodeAllocator.computeRemainder();
  }

private:
  FrameAllocator mCodeAllocator;
};

enum class GeckoHookType { VI_GC, VI_WII };
u32* FindHookInMemory(GeckoHookType type);

} // namespace gecko_jit

namespace kuribo {

typedef gecko_jit::JITEngine kxGeckoJitEngine;

KX_EXPORT
kxGeckoJitEngine* kxCreateJitEngine(u8* memory_begin, u32 memory_size);
KX_EXPORT
void kxDestroyJitEngine(kxGeckoJitEngine* pEngine);

} // namespace kuribo