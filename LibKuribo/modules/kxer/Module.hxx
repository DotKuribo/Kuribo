#pragma once

#include "common.h"
#include "modules/Project.hxx"
#include <memory/heap.hxx>

namespace kuribo {

struct KuriboModule final : public IModule {
  KuriboModule(const u8* buf, const u32 size, mem::Heap* heap);
  int prologue(int type, __kuribo_module_ctx_t* interop) override final;

  mem::unique_ptr<u8[]> mData;
  kuribo_module_prologue mPrologue = nullptr;
};

} // namespace kuribo