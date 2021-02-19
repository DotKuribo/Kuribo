#pragma once

#include "LibKuribo/modules/kxer/Loader.hxx"
#include "common.h"
#include <EASTL/string.h>
#include <memory/heap.hxx>

namespace kuribo {

struct KuriboModuleLoader {
  struct Result {
    bool success;
    eastl::string failure_message;
  };

  static Result tryLoad(const u8* buf, const u32 size, mem::Heap* heap,
                        kxer::LoadedKXE& kxe);
};

} // namespace kuribo