#ifndef KURIBO_ENABLE_LOG
#define KURIBO_ENABLE_LOG 1
#endif

#include "module.hxx"

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


eastl::vector<kuribo::kxer::LoadedKXE>* spLoadedModules;
bool sReloadPending = false;

void LoadModulesOffDisc() {
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

    kuribo::kxer::LoadedKXE& kxe = spLoadedModules->emplace_back();
    kuribo::KuriboModuleLoader::Result result =
        kuribo::KuriboModuleLoader::tryLoad(
            kxmodule.get(), size, &kuribo::mem::GetDefaultHeap(), kxe);
#ifdef KURIBO_MEM_DEBUG
    KURIBO_PRINTF("PROLOGUE: %p\n", kxe.prologue);
#endif

    if (!result.success) {
      eastl::string crash_message = "Failed to load module ";
      crash_message += file.getName();
      crash_message += ":\n";
      crash_message += result.failure_message;
      kuribo::io::OSFatal(crash_message.c_str());
      return; // Not reached
    }

    // Last minute check. There should never be a success state with a null
    // prologue!
    if (kxe.prologue == nullptr) {
      KURIBO_PRINTF("Cannot load %s: Prologue is null\n", file.getName());

      continue;
    }

    modules::DumpInfo(kxe.prologue);
    modules::Enable(kxe.prologue, kxe.data.get());

    KURIBO_PRINTF("FINISHED\n");
  }
}

void Reload() {
  KURIBO_PRINTF("Reloading..\n");
  for (auto& mod : *spLoadedModules) {
    modules::Disable(mod.prologue);
  }
  spLoadedModules->clear();
  LoadModulesOffDisc();
}
void HandleReload() {
  if (sReloadPending) {
    sReloadPending = false;
    Reload();
  }
}

void QueueReload() { sReloadPending = true; }

void PrintLoadedModules() {
  for (auto& mod : *spLoadedModules) {
    modules::DumpInfo(mod.prologue);
  }
}

void SetEventHandlerAddress(u32 address) {
  KURIBO_PRINTF("Setting event handler..\n");
  kuribo::directBranch((void*)address, (void*)(u32)&HandleReload);
}

Arena GetSystemArena() {
#ifdef _WIN32
  constexpr int size = float(923'448) * .7f;
  static char GLOBAL_HEAP[size];

  return {.base_address = &GLOBAL_HEAP[0], .size = sizeof(GLOBAL_HEAP)};
#else
  return HostGetSystemArena();
#endif
}

static void ExposeSdk() {
  kuribo::kxRegisterProcedure("OSReport", FFI_NAME(os_report));
}

static void ExposeGeckoJit() {
  kuribo::kxRegisterProcedure("kxGeckoJitCompileCodes",
                              (u32)&kuribo::kxGeckoJitCompileCodes);
}

static void ExposeModules() {
  kuribo::kxRegisterProcedure("kxSystemReloadAllModules", (u32)&QueueReload);
  kuribo::kxRegisterProcedure("kxSystemSetEventCaller",
                              (u32)&SetEventHandlerAddress);
  kuribo::kxRegisterProcedure("kxSystemPrintLoadedModules",
                              (u32)&PrintLoadedModules);
}

void comet_app_install(void* image, void* vaddr_load, uint32_t load_size) {
  KURIBO_SCOPED_LOG("Installing...");

  {
    const Arena sys_arena = GetSystemArena();
    KURIBO_PRINTF("ARENA STARTS AT %p\n", sys_arena.base_address);
    kuribo::mem::Init(sys_arena.base_address, sys_arena.size, nullptr, 0);
  }

  kuribo::System::createSystem();

  {
    kuribo::SymbolManager::initializeStaticInstance();
    ExposeSdk();
    ExposeGeckoJit();
    ExposeModules();
  }

  kuribo::io::fs::InitFilesystem();

  spLoadedModules = new eastl::vector<kuribo::kxer::LoadedKXE>();

  LoadModulesOffDisc();
}
