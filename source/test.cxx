#include "modules/gecko/Frontend.hpp"

#include <EASTL/array.h>
#include <EASTL/string.h>
#include "system/memory.hxx"

#include "common.h"

#include "api/HostInterop.h"

struct CodePrinter final : public kuribo::gecko::ICodeReceiver {
    void onCodeBegin(eastl::string_view title) override {
        eastl::string to_print(title.begin(), title.size());
        KURIBO_LOG("CXPTR: %x\n", (u32)to_print.c_str());
        KURIBO_LOG("\nCode: %s\n", to_print.c_str());
    }
    void onCodeChunk(u32 chunk) override {
        KURIBO_LOG("Chunk: %x\n", chunk);
    }
    void onParseFail() override {

    }
    void onEnd() override {

    }
};

eastl::array<char, 1024 * 4> heap;

eastl::string_view gCodeExample = "$Proc Mon PAL\n04238F14 4E800020\n04009600 48239100\n$OtherCode\n04238F14 4E800020\n04009600 48239100";
void comet_app_install(void* image, void* vaddr_load, uint32_t load_size) {
    KURIBO_LOG("Hello, world!\n");

    for (auto& c : heap) c = 0;
    
    const auto heap_halfsize = heap.size() / 2;
    kuribo::mem::Init(heap.data(), heap_halfsize, heap.data() + heap_halfsize, heap_halfsize);

    kuribo::gecko::CodeParser parser{ gCodeExample };
    CodePrinter printer;
    parser.parse(printer);
}

KURIBO_SET_OS(0x801A25D0, 0);
