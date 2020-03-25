#pragma once

#include "common.h"
#include "Frontend.hxx"

#include <EASTL/string.h>
#include <EASTL/array.h>

namespace kuribo::gecko {

struct CodeHeader {
    CodeHeader* last;
    CodeHeader* next;
    u32 IsActive;
    u16 ID;
    u16 ErrorType;
};

static_assert(sizeof(CodeHeader) == 16, "Invalid header size..");

struct BlockDebug {
    eastl::string name;
    // eastl::string file;
    u32 file_line;
};

struct CodeBlock {
    BlockDebug debug;
    CodeHeader header;
};

} // namespace kuribo::gecko