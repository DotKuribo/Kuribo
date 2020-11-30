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

#include <jit/engine/compiler.hpp>

#include <modules/SymbolManager.hxx>
#include <modules/kamek/Module.hxx>

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

  kuribo::directBranchEx((void*)0x80102021, (void*)0x80402010, true);
  const auto heap_halfsize = heap.size() / 2;
  kuribo::mem::Init(heap.data(), heap_halfsize, heap.data() + heap_halfsize, heap_halfsize);
  tests::CodeJIT();
  tests::CodeParser();
  if (kuribo::BuildPlatform != kuribo::platform::PC) {
    // tests::CodeParserFromDisc();
  }

  kuribo::System::createSystem();
  kuribo::SymbolManager::initializeStaticInstance();
  kuribo::kxRegisterProcedure("OSReport", FFI_NAME(os_report));
  kuribo::kxRegisterProcedure("kxGeckoJitCompileCodes",
                              (u32)&kuribo::kxGeckoJitCompileCodes);
  KURIBO_LOG("ADDR OF PROJMGR: %p\n",
             &kuribo::System::getSystem().mProjectManager);
  auto our_module = eastl::make_unique<kuribo::KamekModule>("Kuribo/TestModule.kmk");
  if (our_module->mData == nullptr) {
    KURIBO_PRINTF("[KURIBO] Failed to load module\n");
  } else {
    kuribo::System::getSystem().mProjectManager.attachModule(
        eastl::move(our_module));
  }
  typedef f32 (*kxU32toF32_t)(u32);
  u32 in = 50;
  kxU32toF32_t cvt = (kxU32toF32_t)kuribo::kxGetProcedure("kxConvertU32toF32");
  if (cvt == nullptr) {
    KURIBO_LOG("Cannot find function!\n");
  } else {
    f32 out = cvt(in);
    KURIBO_LOG("RESULT: %f\n", out);
  }
}
