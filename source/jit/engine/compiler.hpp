// (So this file is only included once)
#pragma once

#include "common.h"
#include "engine.hpp"

namespace gecko_jit {

bool BeginCodeList(JITEngine& engine);
bool CompileCodeList(JITEngine& engine, const u32* list, size_t size);
bool EndCodeList(JITEngine& engine);

}