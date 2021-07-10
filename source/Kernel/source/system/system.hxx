#pragma once

#include <EASTL/string_view.h>
#include <EASTL/unique_ptr.h>

#include "config.h"
#include "debug.h"
#include "debug/log.h"
#include "types.h"

#include "core/sync.hxx"
#include "util/deferred.hxx"

#include "system/memory.hxx"

namespace kuribo {

using AbortHandler = void (*)(const char* reason);

void DefaultAbortHandler(const char* reason);

extern AbortHandler onAbort;

inline void abort(const char* reason) {
  KURIBO_LOG("---\nkuribo ALERT: ABORTING SYSTEM\n---\n");
  KURIBO_LOG("USER REASON: %s\n", reason);
  KURIBO_LOG("UPTIME: %u SECONDS\n", 0);
  // TODO -- print stack
  onAbort(reason);
}

} // namespace kuribo