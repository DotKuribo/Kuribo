#define KURIBO_ENABLE_LOG

#include "modules/gecko/Frontend.hxx"

#include <EASTL/array.h>
#include <EASTL/string.h>
#include "system/memory.hxx"

#include "common.h"

#include "api/HostInterop.h"
#include "io/io.hxx"

#include "modules/gecko/CodePrinter.hxx"

#include "system/system.hxx"
#include <core/patch.hxx>

#include <io/filesystem.hxx>
#include <io/io.hxx>

void GeckoJIT_RunTests();

namespace tests {
void CodeJIT() {
  {
    KURIBO_SCOPED_LOG("Code Parsing Test");

    //GeckoJIT_RunTests();
  }

  eastl::string_view search_path = "Race/Course/castle_course.szs";
  kuribo::io::fs::InitFilesystem();
  u32 rvl_e, kur_e;
#ifndef _WIN32
  {
    KURIBO_SCOPED_LOG("Revolution Search");

    rvl_e = kuribo::io::dvd::queryEntryNum(search_path.data());
  }
  KURIBO_LOG("REVOLUTION: %u\n", rvl_e);
#endif
  {
    KURIBO_SCOPED_LOG("Kuribo Search");

    kur_e = kuribo::io::fs::Path(search_path).getResolved();
  }
  KURIBO_LOG("KURIBO: %u\n", kur_e);
  {
    kuribo::io::fs::Path p("Race");
    p /= "Course";
    p /= "Beginner_course.szs";
    kur_e = p.getResolved();
  }
  KURIBO_LOG("KURIBO: %u\n", kur_e);

  for (auto it : kuribo::io::fs::RecursiveDirectoryIterator("Pokey")) {
    KURIBO_LOG("How's this? %s\n", it.getName());
  }
}
void CodeParser() {
  KURIBO_SCOPED_LOG("Code Parsing Test");

  // Create a read-only view.
  const eastl::string_view code_example = "$Proc Mon PAL\n042153B8 4E800020\n$OtherCode\n04238F14 4E800020\n04009600 48239100";

  // Construct a lexical parser.
  kuribo::gecko::CodeParser<kuribo::gecko::CodePrinter> parser{ code_example };

  // Attach a temporary CodePrinter to receive parsing actions.
  kuribo::gecko::CodePrinter printer;

  parser.parse(printer);
}
} // namespace tests

eastl::array<char, 1024 * 4 * 4 * 4> heap;
void comet_app_install(void* image, void* vaddr_load, uint32_t load_size) {
  KURIBO_SCOPED_LOG("Installing...");

  heap = {};
#ifndef _WIN32
  for (int i = 0; i < heap.size(); i += 32)
    kuribo::flushAddr(&heap[0] + i);
#endif
  heap = {};


  const auto heap_halfsize = heap.size() / 2;
  kuribo::mem::Init(heap.data(), heap_halfsize, heap.data() + heap_halfsize, heap_halfsize);
  tests::CodeJIT();
  tests::CodeParser();
  if (kuribo::BuildPlatform != kuribo::platform::PC) {
    // tests::CodeParserFromDisc();
  }

  kuribo::System::createSystem();
}
