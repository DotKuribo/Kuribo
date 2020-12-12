#pragma once

#include <stdint.h>

namespace comet {

struct CmxHeader {
  uint32_t magic;             // 00 'CMX0' CoMet eXecutable v0
  uint32_t entry_point_vaddr; // 04
  uint32_t vaddr_load;        // 08
  uint32_t load_size;         // 0C

  uint32_t reserved[4]; // 16B reserved
};

static_assert(sizeof(CmxHeader) == 32,
              "Improperly sized CMX header structure.");

} // namespace comet