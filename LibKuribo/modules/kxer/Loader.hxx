#pragma once

#include <EASTL/string_view.h>
#include <memory/heap.hxx>
#include <modules/kxer/Binary.hxx>

namespace kuribo::kxer {

struct LoadParam {
  eastl::string_view binary;
  mem::Heap* heap = nullptr;
  void** prologueCb = nullptr;
  mem::unique_ptr<u8>* textCb;
};

enum class LoadResult {
  Success,
  MalformedRequest,
  InvalidFileType,
  InvalidVersion,
  InvalidFile,
  BadAlloc,
  BadReloc
};

LoadResult Load(const LoadParam& param);

} // namespace kuribo::kxer