#pragma once

#include "common.h"
#include "engine.hpp"

namespace gecko_jit {

bool BeginCodeList(JITEngine& engine);
bool CompileCodeList(JITEngine& engine, const u32* list, size_t size);
bool EndCodeList(JITEngine& engine);

} // namespace gecko_jit

namespace kuribo {

typedef void (*compiled_function_t)();

// Call this function to execute the code list. (Many codes rely on this being
// once a frame.)
//
// NOTE: Will return NULL if operations failed.
KX_EXPORT
compiled_function_t kxGeckoJitCompileCodes(u8* memory_begin, u32 memory_size,
                                           const u32* list, u32 size);

} // namespace kuribo