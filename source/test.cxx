#include "modules/gecko/Frontend.hxx"

#include <EASTL/array.h>
#include <EASTL/string.h>
#include "system/memory.hxx"

#include "common.h"

#include "api/HostInterop.h"
#include "io/io.hxx"

#include "modules/gecko/CodePrinter.hxx"
#include "modules/gecko/CodeManager.hxx"

#include "system/system.hxx"

namespace tests {
    void CodeParser() {
        KURIBO_SCOPED_LOG("Code Parsing Test");

        // Create a read-only view.
        const eastl::string_view code_example = "$Proc Mon PAL\n042153B8 4E800020\n$OtherCode\n04238F14 4E800020\n04009600 48239100";

        // Construct a lexical parser.
        kuribo::gecko::CodeParser parser{ code_example };

        // Attach a temporary CodePrinter to receive parsing actions.
        kuribo::gecko::CodePrinter printer;
        parser.parse(printer);
    }
    void CodeParserFromDisc() {
        KURIBO_SCOPED_LOG("Code Parsing From Disc Test");

        auto str = kuribo::io::dvd::loadFileString("kuribo_codes.txt");

        // Construct a lexical parser.
        kuribo::gecko::CodeParser parser{ eastl::string_view { str } };

        kuribo::gecko::CodeManager mgr;

        // Attach a temporary CodePrinter to receive parsing actions.
        kuribo::gecko::CodeManagerDelegate printer{ mgr, "kuribo_codes.txt" };
        parser.parse(printer);

        int i = 0;
        for (const auto& block : mgr.blocks) {
            auto& header = block.getCodeBlock().header;
            KURIBO_LOG("Block #%i: %p\n", i++, &header);
            KURIBO_LOG("- last:   %p\n", header.last);
            KURIBO_LOG("- next:   %p\n", header.next);
            KURIBO_LOG("- active: %s\n", header.IsActive ? "true" : "false");
            KURIBO_LOG("- id:     %u\n", static_cast<u16>(header.ID));
            KURIBO_LOG("- err:    %u\n", static_cast<u16>(header.ErrorType));
            // KURIBO_LOG("- file:   %s\n", block->debug.file.c_str());
            KURIBO_LOG("- name:   %s\n", block.getCodeBlock().debug.name.c_str());
            // KURIBO_LOG("- line:   %u\n", block->debug.file_line);
            bool odd = true;
            for (const auto c : block.getData()) {
                KURIBO_LOG_FUNCTION("   %c %08x%c", odd ? '|' : ' ', c, odd ? ' ' : '\n');
                if (c == 0xFE000000) {
                    KURIBO_LOG_FUNCTION("\n");
                    break;
                }
                odd = !odd;
            }
        }
    }
} // namespace tests

eastl::array<char, 1024 * 4 * 4 * 4> heap;
void comet_app_install(void* image, void* vaddr_load, uint32_t load_size) {    
    KURIBO_SCOPED_LOG("Installing...");

    for (auto& c : heap) c = 0;
    
    const auto heap_halfsize = heap.size() / 2;
    kuribo::mem::Init(heap.data(), heap_halfsize, heap.data() + heap_halfsize, heap_halfsize);

    tests::CodeParser();
    if (kuribo::BuildPlatform != kuribo::platform::PC) {
        // tests::CodeParserFromDisc();
    }

    kuribo::System::createSystem();
}

/* Defines the function addresses in PAL MKW */
#define OSReport 0x801A25D0
#define OSFatal 0x801A4EC4
#define DVDConvertPathToEntryNum 0x8015DF4C
#define DVDFastOpen 0x8015E254
#define DVDReadPrio 0x8015E834
#define DVDClose 0x8015E568

/* Sets GC/Wii OS functions. */
KURIBO_SET_OS(OSReport, OSFatal)

/* Sets functions to read from the disc. */
KURIBO_SET_DVD(DVDConvertPathToEntryNum, DVDFastOpen, DVDReadPrio, DVDClose)