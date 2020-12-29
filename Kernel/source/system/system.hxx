#pragma once

#include <EASTL/unique_ptr.h>

#include "config.h"
#include "debug.h"
#include "debug/log.h"
#include "types.h"

#include "core/sync.hxx"
#include "util/deferred.hxx"

#include "modules/Project.hxx"

namespace kuribo {

using AbortHandler = void (*)(const char* reason);

void DefaultAbortHandler(const char* reason);

class System final {
  friend struct DeferredSingleton<System>;

public:
  static System* sInstance;
  static System& getSystem() {
    KURIBO_ASSERT(sInstance);
    return *sInstance;
  }
  static void createSystem() { sInstance = new (&mem::GetHeap(mem::GlobalHeapType::MEM2), 32) System(); }
  static void destroySystem() { delete sInstance; }

  inline void abort(const char* reason) {
    KURIBO_LOG("---\nkuribo ALERT: ABORTING SYSTEM\n---\n");
    KURIBO_LOG("USER REASON: %s\n", reason);
    KURIBO_LOG("UPTIME: %u SECONDS\n", 0);
    // TODO -- print stack
    mAbortHandler(reason);
  }
  void setAbortHandler(AbortHandler handler) { mAbortHandler = handler; }
  bool loadCodeTextFile(const eastl::string_view path);

private:
  AbortHandler mAbortHandler = DefaultAbortHandler;


public:
  System();
  ~System() = default;
};

} // namespace kuribo