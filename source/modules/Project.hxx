#pragma once

#include "config.h"
#include "io/io.hxx"
#include "types.h"

#include "api/Module.h"
#include <EASTL/memory.h>
#include <EASTL/vector.h>
#include <cstdio>

namespace kuribo {

typedef void kuribo_token;

struct IModule {
  virtual ~IModule() = default;
  virtual int prologue(int type, __kuribo_module_ctx_t* interop) = 0;
};
void* interopAlloc(kuribo_token* token, u32 size, u32 align);
void interopFree(kuribo_token* token, void* block);

struct ModuleInstance {
  ModuleInstance(eastl::unique_ptr<IModule> m);
  ~ModuleInstance(); // detach on destruction
  // TODO: Check this
  //	ModuleInstance(ModuleInstance&& other)
  //		: pModule(eastl::move(other.pModule)), mInterop(other.mInterop)
  //	{}

  enum class InteropId { Unitialized = 0, Valid = 'MDIO' };

  eastl::unique_ptr<IModule> pModule;
  __kuribo_module_ctx_t mInterop{};

  bool configured() const;
  bool configure();
  bool attach();
  bool detach();
  bool reload();
  // When loading a new version
  bool transitionTo(eastl::unique_ptr<IModule> pOther);

private:
  bool moduleCall(__KReason t, __kuribo_module_ctx_t* arg);
};

class ProjectManager {
public:
  bool attachModule(eastl::unique_ptr<IModule> module) {
    // ModuleInstance ctor configs/attaches
    mModules.emplace_back(
        eastl::make_unique<ModuleInstance>(std::move(module)));
    return true;
  }

private:
  eastl::vector<eastl::unique_ptr<ModuleInstance>> mModules;
};

} // namespace kuribo