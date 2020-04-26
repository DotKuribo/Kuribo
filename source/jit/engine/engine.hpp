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

private:
  FrameAllocator mCodeAllocator;
};

} // namespace gecko_jit