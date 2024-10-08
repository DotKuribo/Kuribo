#ifndef KURIBO_ENABLE_LOG
#define KURIBO_ENABLE_LOG 1
#define KURIBO_MEM_DEBUG 1
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

#include <modules/SymbolManager.hxx>
#include <modules/kxer/Module.hxx>

#include "FallbackAllocator/free_list_heap.hxx"

#include "LibKuribo/compr.hxx"

kuribo::DeferredInitialization<eastl::vector<kuribo::kxer::LoadedKXE>>
    spLoadedModules;
bool sReloadPending = false;

void LoadModuleFile(const char* file_name, u8* file, u32 size,
                    kuribo::mem::Heap& heap) {
  kuribo::kxer::LoadedKXE& kxe = spLoadedModules->emplace_back();
  kuribo::KuriboModuleLoader::Result result =
      kuribo::KuriboModuleLoader::tryLoad(file, size, &heap, kxe);
#ifdef KURIBO_MEM_DEBUG
  KURIBO_PRINTF("PROLOGUE: %p\n", kxe.prologue);
#endif

  if (!result.success) {
    eastl::string crash_message = "Failed to load module ";
    crash_message += file_name;
    crash_message += ":\n";
    crash_message += result.failure_message;
    kuribo::io::OSFatal(crash_message.c_str());
    return; // Not reached
  }

  // Last minute check. There should never be a success state with a null
  // prologue!
  if (kxe.prologue == nullptr) {
    KURIBO_PRINTF("Cannot load %s: Prologue is null\n", file_name);

    return;
  }

  modules::DumpInfo(kxe.prologue);
  modules::Enable(kxe.prologue, kxe.data.get());
}

void LoadModulesOffDisc(kuribo::mem::Heap& file_heap,
                        kuribo::mem::Heap& module_heap) {
  auto fs =
      kuribo::io::fs::DiscFileSystem(kuribo::io::fs::rvl_os_filesystem_root{});

  auto path = kuribo::io::fs::Path(fs, "Kuribo!/Mods/");

  if (path.getNode() == nullptr) {
    KURIBO_PRINTF("Failed to resolve Mods folder.\n");

    kuribo::io::OSFatal("Kuribo: Missing folder Kuribo!/Mods/");

    return;
  }

  KURIBO_PRINTF("Folder: %s\n", path.getName());

  for (auto file : kuribo::io::fs::RecursiveDirectoryIterator(path)) {
    KURIBO_PRINTF("FILE: %s\n", file.getName());

    int size, rsize;
    auto kxmodule = kuribo::io::dvd::loadFile(file, &size, &rsize, &file_heap);

    if (kxmodule.get() == nullptr) {
      KURIBO_PRINTF("Failed to read off disc..\n");
      continue;
    }

    KURIBO_PRINTF("Loaded module. Size: %i, rsize: %i\n", size, rsize);

    LoadModuleFile(file.getName(), kxmodule.get(), size, module_heap);

    KURIBO_PRINTF("FINISHED\n");
  }
}

void Reload() {
  KURIBO_PRINTF("Reloading..\n");
  for (auto& mod : *spLoadedModules) {
    modules::Disable(mod.prologue);
  }
  spLoadedModules->clear();
  // LoadModulesOffDisc();
}
void HandleReload() {
  if (sReloadPending) {
    sReloadPending = false;
    Reload();
  }
}

void QueueReload() { sReloadPending = true; }

#ifndef __VERSION__
#define __VERSION__ ""
#endif

void PrintLoadedModules() {
  KURIBO_PRINTF("&a---Kuribo ("
                "&9" __VERSION__ ", built " __DATE__ " at " __TIME__
                "&a)---&f\n");
  for (auto& mod : *spLoadedModules) {
    modules::DumpInfo(mod.prologue);
  }
}

void SetEventHandlerAddress(u32 address) {
  KURIBO_PRINTF("Setting event handler..\n");
  kuribo::directBranch((void*)address, (void*)(u32)&HandleReload);
}

struct __kuribo_guard {
  u8 is_init; /* This byte must be here, the rest are free */
  u8 _pad[3];

  /* On PPC, we will just disable multitasking */
  u32 msr_save;
};
static_assert(sizeof(__kuribo_guard) == 8);

// This is actually unsafe because we could interrupt while doing this. The SDK
// interrupt handler specifically handles OSDisableInterrupts and we should use
// that directly.
extern "C" int __cxa_guard_acquire(__kuribo_guard* guard) {
  const u32 msr = __kuribo_mfmsr();
  __kuribo_mtmsr(msr & ~0x8000);

  guard->is_init = 0;
  guard->msr_save = msr;

  return 1;
}
extern "C" void __cxa_guard_release(__kuribo_guard* guard) {
  __kuribo_mtmsr(guard->msr_save);
}

static void ExposeSdk(kuribo::SymbolManager& sym) {
  sym.registerProcedure("OSReport", FFI_NAME(os_report));
  sym.registerProcedure("__cxa_guard_acquire", (u32)&__cxa_guard_acquire);
  sym.registerProcedure("__cxa_guard_release", (u32)&__cxa_guard_release);
}

static void ExposeModules(kuribo::SymbolManager& sym) {
  sym.registerProcedure("kxSystemReloadAllModules", (u32)&QueueReload);
  sym.registerProcedure("kxSystemInstallReloadHandler",
                        (u32)&SetEventHandlerAddress);
  sym.registerProcedure("kxSystemPrintLoadedModules", (u32)&PrintLoadedModules);
  sym.registerProcedure("_ZN9ScopedLog10sLogIndentE",
                        (u32)&ScopedLog::sLogIndent);

  // kuribo_symbols.h
  sym.registerProcedure("kxRegisterProcedureEx",
                        (u32)&kuribo::kxRegisterProcedureEx);
  sym.registerProcedure("kxGetProcedureEx", (u32)&kuribo::kxGetProcedureEx);
  sym.registerProcedure("kxRegisterProcedure",
                        (u32)&kuribo::kxRegisterProcedure);
  sym.registerProcedure("kxGetProcedure", (u32)&kuribo::kxGetProcedure);

  // kuribo_szs.h
  sym.registerProcedure("kxSzsCheckCompressed",
                        (u32)&kuribo::kxSzsCheckCompressed);
  sym.registerProcedure("kxSzsDecodedSize", (u32)&kuribo::kxSzsDecodedSize);
  sym.registerProcedure("kxSzsDecodeIntoV1", (u32)&kuribo::kxSzsDecodeIntoV1);
}

kuribo::mem::Heap* sModulesHeap;

void comet_app_install(void* image, void* vaddr_load, uint32_t load_size) {
  KURIBO_SCOPED_LOG("Installing...");

  {
    // Initialize fallback heap, 1KB in size
    kuribo::mem::Init();

    sModulesHeap = HostGetModuleAllocator();
    KURIBO_ASSERT(sModulesHeap);
  }

  {
    auto& sym = kuribo::SymbolManager::initializeStaticInstance(*sModulesHeap);
    ExposeSdk(sym);
    ExposeModules(sym);
  }

  spLoadedModules.initialize();
  LoadModulesOffDisc(*sModulesHeap, *sModulesHeap);
}
