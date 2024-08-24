/*
 * @file  kuribo_szs.h
 *
 * @brief Implementation of SZS (YAZ0) compression for the Kuribo mod loader.
 *
 * This API is introduced to support SZS (YAZ0) compression, a format widely
 * used in Nintendo games.
 *
 * Why expose this API from the Kernel?
 *
 * 1. SZS (YAZ0) compression is ubiquitous in the GC/Wii games Kuribo aims to
 *    support.
 *
 * 2. YAZ0 handling in the kernel doubles as the compression backend for .kxe
 *    files, which are the custom modules supported by the Kuribo kernel.
 *
 * 3. Existing Nintendo implementations, like the one in Mario Kart Wii, are
 *    insecure and susceptible to buffer overflow vulnerabilities from malformed
 *    files. This API aims to provide a secure alternative.
 *
 * 4. Existing implementations for compression especially are quite slow. This
 *    API offers a faster alternative.
 *
 * 5. This surface provides a common interface for game-agnostic modules to
 *    interface with compression. In kernels for specific games, these routines
 *    can route to game implementations if desired.
 */
#pragma once

#include "kuribo_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  KX_COMPRESSION_NONE,
  KX_COMPRESSION_SZS,
} kxComprType;

// Values of kxComprType
KX_API u32 kxSzsCheckCompressed(const void* buf, u32 buf_len);

KX_API u32 kxSzsDecodedSize(const void* buf, u32 buf_len);

// returns nullptr if OK, static error string otherwise
KX_API const char* kxSzsDecodeIntoV1(void* dst, u32 dstLen, const void* src,
                                     u32 srcLen);

#ifdef __cplusplus
}
#endif
