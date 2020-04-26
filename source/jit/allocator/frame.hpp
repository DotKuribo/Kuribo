#pragma once

#include "common.h"

// We have a shrinking frame. From memory region A to B:
// A --------- B
// We can allocate (but not free) off of either end
// A XXX----YY B
// The benefit here is that we can efficiently fill in a block of memory without
// knowing the relative sizes of X/Y. In the compiler, the two big regions will
// be
// - Per frame functions
// - Referenced, call-on-demand functions
// So our region will look as follows:
// MEMMORY_BEGIN:
// (compiled) 04...
// (compiled) 02...
// blr
// |
// V expand downward
//
// Free space
//
// ^
// | expand upward
// C0 body
// blr
// C2 body
// blr
// MEMORY_END

namespace gecko_jit {

class FrameAllocator {
public:
  FrameAllocator(u8* memory_begin, size_t size)
      : begin(memory_begin), end(memory_begin + size), up_it(begin),
        down_it(end) {}

  u8* alloc(size_t size, bool head = true) {
    if (head) {
      u8* dst = up_it;
      up_it += size;
      return dst;
    } else {
      return down_it -= size;
    }
  }

private:
  u8* begin;
  u8* end;

  u8* up_it;
  u8* down_it;
};

} // namespace gecko_jit