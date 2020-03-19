#include "modules/gecko/Frontend.hpp"
#include <cstdio>
#include <string>

struct CodePrinter final : public kuribo::gecko::ICodeReceiver {
    void onCodeBegin(eastl::string_view title) override {
        std::string to_print(title.begin(), title.size());
        printf("\nCode: %s\n", to_print.c_str());
    }
    void onCodeChunk(u32 chunk) override {
        printf("Chunk: %x\n", chunk);
    }
    void onParseFail() override {

    }
    void onEnd() override {

    }
};
eastl::string_view gCodeExample = "$Proc Mon PAL\n04238F14 4E800020\n04009600 48239100\n$OtherCode\n04238F14 4E800020\n04009600 48239100";
int main() {
    kuribo::gecko::CodeParser parser{ gCodeExample };
    CodePrinter printer;
    parser.parse(printer);
}