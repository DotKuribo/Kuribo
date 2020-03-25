#pragma once

#include "common.h"
#include "Frontend.hxx"

#include "CodeHandler.hxx"

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/optional.h>

namespace kuribo::gecko {


class ExpandingCodeBuffer {
public:
    ExpandingCodeBuffer() : mRawData(sizeof(CodeBlock) / 4) {}

    CodeBlock& getCodeBlock() {
        if (mRawData.size() * 4 < sizeof(CodeBlock)) {
            KURIBO_LOG("DATASIZE: %u, BlockSize: %u\n", mRawData.size() * 4, sizeof(CodeBlock));
        }
        KURIBO_ASSERT(mRawData.size() * 4 >= sizeof(CodeBlock));

        return reinterpret_cast<CodeBlock&>(mRawData[0]);
    }
    
    const CodeBlock& getCodeBlock() const {
        KURIBO_ASSERT(mRawData.size() * 4>= sizeof(CodeBlock));

        return reinterpret_cast<const CodeBlock&>(mRawData[0]);
    }

    // TODO: We can scan the code file first to prevent reallocations
    void pushSimpleBlock(u32 block) {
        mRawData.push_back(block);
    }
    const eastl::vector<u32>& getData() const { return mRawData; }

private:
    eastl::vector<u32> mRawData;
};

class CodeManager {
public:
    ExpandingCodeBuffer& getAt(std::size_t x) { return blocks.at(x); }
    ExpandingCodeBuffer& getLast() {
        return getAt(blocks.size() - 1);
    }
    void pushBlock(ExpandingCodeBuffer block) {
        blocks.emplace_back(eastl::move(block));
    }
    std::size_t getNum() const { return blocks.size(); }

// private:
    eastl::vector<ExpandingCodeBuffer> blocks;
};

class CodeManagerDelegate final : public kuribo::gecko::ICodeReceiver {
public:
    void onCodeBegin(eastl::string_view title) override {
        eastl::string name(title.begin(), title.size());
        KURIBO_LOG("Parsing code: %s\n", name.c_str());

        if (cur.has_value()) endCode();

        KURIBO_ASSERT(!cur.has_value());
        cur.emplace();
        KURIBO_ASSERT(cur.has_value());
        auto& header = cur->getCodeBlock().header;
        header.ID = manager.getNum();
        header.IsActive = true;
        header.ErrorType = 0;
        header.last = !manager.getNum() ? nullptr : &manager.getLast().getCodeBlock().header;
        header.next = nullptr;

        auto& debug = cur->getCodeBlock().debug;
        debug.name = name;
        // debug.file = fileName;
        debug.file_line = 0;
    }
    void onCodeChunk(u32 chunk) override {
        //  KURIBO_LOG_FUNCTION("%08x%c", chunk, odd ? ' ' : '\n');
        //  odd = !odd;

        cur->pushSimpleBlock(chunk);
    }
    void onParseFail() override { /* TODO */}
    void onEnd() override { endCode(); }

    void endCode() {
        KURIBO_ASSERT(cur.has_value());
        cur->pushSimpleBlock(0xFE000000);
        cur->pushSimpleBlock(0x00000000);
        manager.pushBlock(eastl::move(cur.value()));
        cur = {};
        cur.reset();

        if (manager.getNum() > 1) {
            manager.getAt(manager.getNum() - 2).getCodeBlock().header.next = &manager.getLast().getCodeBlock().header;
        }
        KURIBO_ASSERT(!cur.has_value());
    }

    CodeManagerDelegate(CodeManager& mgr, const char* fileName) : manager(mgr) {}

private:
    eastl::string fileName;
    CodeManager& manager;
    eastl::optional<ExpandingCodeBuffer> cur = eastl::nullopt;
    // bool odd = true;
};

} // namespace kuribo::gecko