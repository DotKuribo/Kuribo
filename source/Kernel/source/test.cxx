#define KURIBO_ENABLE_LOG

#include "system/memory.hxx"
#include <EASTL/array.h>
#include <EASTL/string.h>

#include "common.h"

#include "api/HostInterop.h"
#include "io/io.hxx"

#include "system/system.hxx"
#include <core/patch.hxx>

#include <LibKuribo/filesystem.hxx>
#include <io/io.hxx>

#include "GeckoJIT/engine/compiler.hpp"

#include <modules/SymbolManager.hxx>
#include <modules/kxer/Module.hxx>

void PrintModuleInfo(kuribo_module_prologue prologue) {
  __kuribo_module_ctx_t ctx;
  ctx.core_version = KURIBO_CORE_VERSION;

  __kuribo_simple_meta_v0 meta;
  ctx.udata.fillin_meta = &meta;

  prologue(KURIBO_REASON_INQUIRE_META_DESC, &ctx);

  KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");
  KURIBO_PRINTF("[KURIBO] Loading module\n");
  KURIBO_PRINTF("         Name:     \t%s\n", meta.module_name);
  KURIBO_PRINTF("         Author:   \t%s\n", meta.module_author);
  KURIBO_PRINTF("         Version:  \t%s\n", meta.module_version);
  KURIBO_PRINTF("                     \n");
  KURIBO_PRINTF("         Built:    \t%s\n", meta.build_date);
  KURIBO_PRINTF("         Compiler: \t%s\n", meta.compiler_name);
  KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");
}

void LinkModule(kuribo_module_prologue prologue) {
  __kuribo_module_ctx_t ctx;
  ctx.core_version = KURIBO_CORE_VERSION;

  ctx.get_procedure = nullptr;
  ctx.register_procedure = nullptr;

  prologue(KURIBO_REASON_LOAD, &ctx);
}

void UnlinkModule(kuribo_module_prologue prologue) {
  __kuribo_module_ctx_t ctx;
  ctx.core_version = KURIBO_CORE_VERSION;

  prologue(KURIBO_REASON_UNLOAD, &ctx);
}

void comet_app_install(void* image, void* vaddr_load, uint32_t load_size) {
  KURIBO_SCOPED_LOG("Installing...");

  constexpr u32 size = 923'448 / 2;

#ifdef _WIN32
  static char GLOBAL_HEAP[size];
  char* base_addr = &GLOBAL_HEAP[0];
#else
  void* our_heap = *(void**)(0x802A40A4);
  char* base_addr =
      ((char* (*)(u32, void*, u32))0x80229DE0)(size, our_heap, 32);
  KURIBO_PRINTF("ALLOCATED BLOCK AT: %p\n", base_addr);
#endif

  kuribo::mem::Init(base_addr, size, nullptr, 0);
  // kuribo::mem::AddRegion((void*)0x8042EB00, 923448, false);

  kuribo::System::createSystem();
  kuribo::SymbolManager::initializeStaticInstance();
  kuribo::kxRegisterProcedure("OSReport", FFI_NAME(os_report));
  kuribo::kxRegisterProcedure("kxGeckoJitCompileCodes",
                              (u32)&kuribo::kxGeckoJitCompileCodes);

  kuribo::io::fs::InitFilesystem();

  auto path = kuribo::io::fs::Path("Kuribo!/Mods/");

  if (path.getNode() == nullptr) {
    KURIBO_PRINTF("Failed to resolve Mods folder.\n");
    return;
  }

  KURIBO_PRINTF("Folder: %s\n", path.getName());

  for (auto file : kuribo::io::fs::RecursiveDirectoryIterator(path)) {
    KURIBO_PRINTF("FILE: %s\n", file.getName());

    int size, rsize;
    auto kxmodule =
        kuribo::io::dvd::loadFile(file.getResolved(), &size, &rsize);

    if (kxmodule == nullptr) {
      KURIBO_PRINTF("Failed to read off disc..\n");
      continue;
    }

    KURIBO_PRINTF("Loaded module. Size: %i, rsize: %i\n", size, rsize);

    auto* module = new kuribo::KuriboModule(kxmodule.get(), size,
                                            &kuribo::mem::GetDefaultHeap());
    KURIBO_PRINTF("PROLOGUE: %p\n", module->mKXE.prologue);

    PrintModuleInfo(module->mKXE.prologue);
    LinkModule(module->mKXE.prologue);

    KURIBO_PRINTF("FINISHED\n");
  }
}
