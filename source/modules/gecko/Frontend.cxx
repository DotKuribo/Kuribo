#include "config.h"
#include "Frontend.hxx"
#include "debug/log.h"

namespace kuribo::gecko {


bool isLineEnd(char in) { return in == '\n' || in == '\r'; };

void CodeParser::parse(ICodeReceiver& out) {
    while (mCursor < mData.size()) {
        if (mData[mCursor] == '$') {
            const auto titlebegin = ++mCursor;
            while (mCursor < mData.size()) {
                if (isLineEnd(mData[mCursor])) {
                    break;
                }
                ++mCursor;
            }
            out.onCodeBegin({mData.data() + titlebegin, mCursor-titlebegin});
        } else if (isLineEnd(mData[mCursor]) || mData[mCursor] == ' ') {
            ++mCursor;
        } else {
            const auto chunkbegin = mCursor;
            while (mCursor < mData.size()) {
                if (isLineEnd(mData[mCursor]) || mData[mCursor] == ' ') {
                    break;
                }
                ++mCursor;
            }
            eastl::string_view chunk {mData.data() + chunkbegin, mCursor - chunkbegin};
            KURIBO_ASSERT(chunk.size() == 8);
            if (chunk.size() != 8) {
                out.onParseFail();
                return;
            }
            u32 chunk_data = 0;
            for (char c : chunk) {
                u8 val = 0;
                if (c >= '0' && c <= '9') val = c - '0';
                else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
                chunk_data <<= 4;
                chunk_data |= val & 0xf; 
            }
            out.onCodeChunk(chunk_data);
        }
    }
    out.onEnd();
}

} // namespace kuribo::gecko