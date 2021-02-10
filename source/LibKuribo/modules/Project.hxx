#pragma once

#include "config.h"
#include "types.h"
#include <EASTL/memory.h>
#include <EASTL/vector.h>
#include <cstdio>
#include <security/Canary.hxx>
#include <memory/heap.hxx>
#include <sdk/kuribo_sdk.h>

namespace kuribo {

class IModule {
public:
  virtual ~IModule() = default;
  virtual int prologue(int type, __kuribo_module_ctx_t* interop) = 0;
};

// A module may never be freed
struct ModuleInstance final {
  ModuleInstance(mem::unique_ptr<IModule> m);
  ~ModuleInstance(); // detach on destruction

  mem::unique_ptr<IModule> pModule;
  CanaryObject<__kuribo_module_ctx_t> mInterop{};

  bool configured() const;
  bool configure();
  bool attach();
  bool detach();
  bool reload();
  // When loading a new version
  bool transitionTo(mem::unique_ptr<IModule> pOther);

private:
  bool moduleCall(__KReason t);
};

} // namespace kuribo