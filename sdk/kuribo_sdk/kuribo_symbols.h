/*
 * @file  kuribo_symbols.h
 *
 * @brief Exposes the kernel symbols APIs. These may be useful if you are using
 *        the SDK via the `KXER` path, which natively supports function imports
 *        without routing through the global context function pointer table.
 */
#pragma once

#include "kuribo_types.h"

/**
 * @brief Registers a symbol to the global symbol hashtable. This version is
 *        faster than `kxRegisterProcedure` (avoiding runtime hashing) and
 *        supports non-null-terminated strings.
 *
 * @param symbol_str: Pointer to the symbol name. Need not be null-terminated.
 *
 * @param symbol_strlen: Length of symbol name
 *
 * @param value: Address of procedure
 *
 * @param crc32_hint Provide a CRC32 of the symbol. This can save time in the
 *                   implementation (especially if you can compute this at
 *                   compile-time with a constexpr hashing implementation), but
 *                   may not be used in future kernel versions using different
 *                   hashing algorithms, so the full symbol name is still
 *                   required! Set to 0 if no hint is available.
 */
KX_API void kxRegisterProcedureEx(const char* symbol_str, u32 symbol_strlen,
                                  void* value, u32 crc32_hint);

/**
 * @brief Retrieves a symbol from the global symbol hashtable.
 *
 * @param symbol_str: Pointer to the symbol name. Need not be null-terminated.
 *
 * @param symbol_strlen: Length of symbol name
 *
 * @param crc32_hint Provide a CRC32 of the symbol. This can save time in the
 *                   implementation (especially if you can compute this at
 *                   compile-time with a constexpr hashing implementation), but
 *                   may not be used in future kernel versions using different
 *                   hashing algorithms, so the full symbol name is still
 *                   required! Set to 0 if no hint is available.
 */
KX_API void* kxGetProcedureEx(const char* symbol_str, u32 symbol_strlen,
                              u32 crc32_hint);

/**
 * @brief Registers a symbol to the global symbol hashtable.
 *
 * @param symbol Null-terminated string.
 * @param value  Address of procedure
 */
KX_API void kxRegisterProcedure(const char* symbol, u32 value);

/**
 * @brief Registers a symbol to the global symbol hashtable.
 *
 * @param symbol Null-terminated string.
 * @param value  Address of procedure
 */
KX_API u32 kxGetProcedure(const char* symbol);
