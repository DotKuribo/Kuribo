#include <types.h>

namespace kuribo {

// Values of kxComprType
u32 kxSzsCheckCompressed(const void* buf, u32 buf_len);

u32 kxSzsDecodedSize(const void* buf, u32 buf_len);

// returns nullptr if OK, static error string otherwise
const char* kxSzsDecodeIntoV1(u8* dst, u32 dstLen, const u8* src, u32 srcLen);

} // namespace kuribo
