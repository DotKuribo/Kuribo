#define KURIBO_ENABLE_ASSERT

#include "Common/debug/assert.h"
#include <types.h>

namespace kuribo {

// Values of kxComprType
u32 kxSzsCheckCompressed(const void* buf, u32 buf_len) {
  if (buf_len < 8) {
    return 0;
  }
  // MKW check only considers first 3 bytes
  if ((*reinterpret_cast<const u32*>(buf) >> 8) != 'YAZ') {
    return 0;
  }
  return 1;
}

u32 kxSzsDecodedSize(const void* buf, u32 buf_len) {
  if (kxSzsCheckCompressed(buf, buf_len) == 0) {
    return 0;
  }
  const u8* src = reinterpret_cast<const u8*>(buf);
  return (static_cast<u32>(src[4]) << 24) | (static_cast<u32>(src[5]) << 16) |
         (static_cast<u32>(src[6]) << 8) | static_cast<u32>(src[7]);
}

const char* kxSzsDecodeIntoV1(u8* dst, u32 dstLen, const u8* src, u32 srcLen) {
  const u8* srcBak = src;
  const u8* srcEnd = src + srcLen;
  u8* dstEnd = dst + dstLen;
  u8 groupHead = 0;
  int groupHeadLen = 0;

  KURIBO_ASSERT(kxSzsCheckCompressed(src, srcLen) == 1);
  KURIBO_ASSERT(dstLen >= kxSzsDecodedSize(src, srcLen));

  while (src < srcEnd && dst < dstEnd) {
    if (!groupHeadLen) {
      groupHead = *src++;
      groupHeadLen = 8;
    }

    --groupHeadLen;
    if (groupHead & 0x80) {
      *dst++ = *src++;
    } else {
      const u8 b1 = *src++;
      const u8 b2 = *src++;

      const u8* copySrc = dst - ((b1 & 0x0f) << 8 | b2) - 1;

      int n = b1 >> 4;

      if (!n) {
        n = *src++ + 0x12;
      } else {
        n += 2;
      }
      KURIBO_ASSERT(n >= 3 && n <= 0x111);

      if (copySrc < srcBak) {
        return "Malformed YAZ0 data: Attempted to read before start of buffer";
      }
      if (dst + n > dstEnd) {
        return "Malformed YAZ0 data: Attempted to write past end of buffer";
      }

      while (n-- > 0) {
        *dst++ = *copySrc++;
      }
    }

    groupHead <<= 1;
  }

  KURIBO_ASSERT(src <= srcEnd);
  KURIBO_ASSERT(dst <= dstEnd);

  return "";
}

} // namespace kuribo