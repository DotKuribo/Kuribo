#pragma once

#include "types.h"

namespace kuribo::mem {

struct Heap {
  virtual void* alloc(u32 size, u32 align = 8) noexcept = 0;
  virtual void free(void* ptr) noexcept = 0;
};

} // namespace kuribo::mem