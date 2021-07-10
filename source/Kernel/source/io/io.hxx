#pragma once

#include "api/HostInterop.h"
#include "common.h"
#include "filesystem.hxx"
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
  void* result_callback;
};
namespace dvd {

using entry = s32;

inline bool open(low_dvd_Handle& handle, const fs::Path& path) {
  if (!path.getNode())
    return false;

  {
    if (path.isFolder())
      return false;

    {
      const auto& file_info = path.getNode()->file;

      handle.addr = file_info.offset;
      handle.len = file_info.size;
    }
  }

  handle.result_callback = nullptr;
  handle._00[3] = 0;

  return true;
}
inline s32 read(low_dvd_Handle& handle, void* buf, int len, int offset) {
  return ((s32(*)(low_dvd_Handle*, void*, int, int, int))FFI_NAME(
      dvd_read_prio))(&handle, buf, len, offset, 2);
}
inline bool close(low_dvd_Handle& handle) {
  return ((s32(*)(low_dvd_Handle*))FFI_NAME(dvd_close))(&handle);
}

struct Handle : public low_dvd_Handle {
  inline Handle(const fs::Path& path) {
    KURIBO_ASSERT(path.getNode() && "Cannot find file on disc");
    if (!path.getNode())
      return;

    bOpened = open(*this, path);
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
inline mem::unique_ptr<u8> loadFile(const fs::Path& path, int* size, int* rsize,
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

  return mem::unique_ptr<u8>(buf, heap ? heap : &kuribo::mem::GetDefaultHeap());
}

inline eastl::string loadFileString(const fs::Path& path) {
  Handle file(path);
  KURIBO_ASSERT(file.opened());
  if (!file.opened())
    return "";

  mem::unique_ptr<u8> tmp{new (&kuribo::mem::GetDefaultHeap(), 32)
                              u8[file.getRoundedSize()],
                          &kuribo::mem::GetDefaultHeap()};
  file.read(tmp.get(), file.getRoundedSize(), 0);

  return {reinterpret_cast<char*>(tmp.get()), file.getRealSize()};
}
} // namespace dvd

} // namespace kuribo::io