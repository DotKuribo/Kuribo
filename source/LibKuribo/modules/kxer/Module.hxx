#pragma once

#include "LibKuribo/modules/kxer/Loader.hxx"
#include "common.h"
#include <memory/heap.hxx>

namespace kuribo {

struct KuriboModule final {
  KuriboModule(const u8* buf, const u32 size, mem::Heap* heap);

  kxer::LoadedKXE mKXE;
};

} // namespace kuribo