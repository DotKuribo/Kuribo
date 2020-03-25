#pragma once

#include <EASTL/string_view.h>
#include "types.h"
#include "debug/assert.h"

namespace kuribo::gecko {

struct ICodeReceiver {
    virtual ~ICodeReceiver() = default;
    
    virtual void onCodeBegin(eastl::string_view title) = 0;
    virtual void onCodeChunk(u32 chunk) = 0;
    virtual void onEnd() = 0;
    virtual void onParseFail() = 0; // TODO: Accept position
};

class CodeParser {
public:
    CodeParser(eastl::string_view data)
        : mData(data)
    {}
    ~CodeParser() = default;

    void parse(ICodeReceiver& out);
private:
    eastl::string_view mData;
    u32 mCursor = 0;
};

} // namespace kuribo::gecko