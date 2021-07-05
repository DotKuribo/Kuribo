#pragma once

#include "common.h"
#include "sdk/kuribo_sdk.h"
#include <optional>

namespace modules {

inline std::optional<__kuribo_simple_meta_v0>
Inspect(kuribo_module_prologue prologue) {
  KURIBO_ASSERT(prologue != nullptr);

  __kuribo_module_ctx_t ctx;
  ctx.core_version = KURIBO_CORE_VERSION;

  __kuribo_simple_meta_v0 meta;
  ctx.udata.fillin_meta = &meta;

  __KExit result =
      static_cast<__KExit>(prologue(KURIBO_REASON_INQUIRE_META_DESC, &ctx));

  if (result != KURIBO_EXIT_SUCCESS)
    return std::nullopt;

  return meta;
}

inline bool DumpInfo(kuribo_module_prologue prologue) {
  KURIBO_ASSERT(prologue != nullptr);

  const auto meta = Inspect(prologue);

  if (!meta) {
    KURIBO_PRINTF("[KURIBO] Failed to inspect module %p\n", prologue);
    return false;
  }

  KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");
  KURIBO_PRINTF("[KURIBO] Loading module\n");
  KURIBO_PRINTF("         Name:     \t%s\n", meta->module_name);
  KURIBO_PRINTF("         Author:   \t%s\n", meta->module_author);
  KURIBO_PRINTF("         Version:  \t%s\n", meta->module_version);
  KURIBO_PRINTF("                     \n");
  KURIBO_PRINTF("         Built:    \t%s\n", meta->build_date);
  KURIBO_PRINTF("         Compiler: \t%s\n", meta->compiler_name);
  KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");

  return true;
}

inline bool Enable(kuribo_module_prologue prologue, void* start_address) {
  KURIBO_ASSERT(prologue != nullptr);
  KURIBO_ASSERT(start_address != nullptr);

  __kuribo_module_ctx_t ctx;
  ctx.core_version = KURIBO_CORE_VERSION;

  ctx.get_procedure = nullptr;
  ctx.register_procedure = nullptr;

  ctx.start_address = reinterpret_cast<char*>(start_address);

  const int result = prologue(KURIBO_REASON_LOAD, &ctx);

  return static_cast<__KExit>(result) == KURIBO_EXIT_SUCCESS;
}

inline bool Disable(kuribo_module_prologue prologue) {
  KURIBO_ASSERT(prologue != nullptr);

  __kuribo_module_ctx_t ctx;
  ctx.core_version = KURIBO_CORE_VERSION;

  const int result = prologue(KURIBO_REASON_UNLOAD, &ctx);

  return static_cast<__KExit>(result) == KURIBO_EXIT_SUCCESS;
}

} // namespace modules