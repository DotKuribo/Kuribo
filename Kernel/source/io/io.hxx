#pragma once

#include "api/HostInterop.h"
#include "common.h"
#include "system/memory.hxx"

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

namespace kuribo::io {

static inline void OSFatal(const char* msg, u32 fg = -1, u32 bg = 0) {
  ((void (*)(u32*, u32*, const char*))FFI_NAME(os_fatal))(&fg, &bg, msg);
}
struct low_dvd_Handle {
  u32 _00[12];
  u32 addr;
  u32 len;
  u32 _38;
};
namespace dvd {
using entry = s32;
constexpr bool entryValid(entry e) { return e >= 0; }
inline entry queryEntryNum(const char* path) {
  return ((s32(*)(const char*))FFI_NAME(dvd_path_to_entrynum))(path);
}
inline bool open(low_dvd_Handle& handle, entry ent) {
  return ((s32(*)(s32, low_dvd_Handle*))FFI_NAME(dvd_fast_open))(ent, &handle);
}
inline s32 read(low_dvd_Handle& handle, void* buf, int len, int offset) {
  return ((s32(*)(low_dvd_Handle*, void*, int, int, int))FFI_NAME(
      dvd_read_prio))(&handle, buf, len, offset, 2);
}
inline bool close(low_dvd_Handle& handle) {
  return ((s32(*)(low_dvd_Handle*))FFI_NAME(dvd_close))(&handle);
}

struct Handle : public low_dvd_Handle {
  inline Handle(const char* path) {
    const auto ent = queryEntryNum(path);
    KURIBO_ASSERT(ent >= 0 && "Cannot find file on disc");
    if (ent < 0)
      return;

    bOpened = open(*this, ent);
    KURIBO_ASSERT(bOpened && "Cannot open file");
  }
  inline ~Handle() {
    if (bOpened)
      close(*this);
  }
  inline u32 getRoundedSize() { return (len + 0x1f) & ~0x1f; }
  inline u32 getRealSize() { return len; }
  inline bool read(void* buf, int size = 0, int offset = 0) {
    KURIBO_ASSERT(bOpened && "Attempted to read from an unopened file.");
    if (!bOpened)
      return false;

    return dvd::read(*this, buf, size, offset);
  }
  bool opened() const { return bOpened; }
  bool bOpened = false;
};
inline eastl::unique_ptr<u8[]> loadFile(const char* path, int* size, int* rsize,
                                        kuribo::mem::Heap* heap = nullptr) {
  Handle file(path);
  if (!file.bOpened)
    return nullptr;

  u8* buf = new (heap ? heap : &kuribo::mem::GetDefaultHeap(), 32)
      u8[file.getRoundedSize()];
  KURIBO_ASSERT(buf && "Cannot allocate buffer.");

  if (rsize)
    *rsize = file.getRealSize();
  if (size)
    *size = file.getRoundedSize();

  file.read(buf, file.getRoundedSize(), 0);

  return eastl::unique_ptr<u8[]>(buf);
}
inline eastl::string loadFileString(const char* path) {
  Handle file(path);
  KURIBO_ASSERT(file.opened());
  if (!file.opened())
    return "";

  eastl::unique_ptr<u8[]> tmp{new (nullptr, 32) u8[file.getRoundedSize()]};
  file.read(tmp.get(), file.getRoundedSize(), 0);

  return {reinterpret_cast<char*>(tmp.get()), file.getRealSize()};
}
} // namespace dvd

} // namespace kuribo::io