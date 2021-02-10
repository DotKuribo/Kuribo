#pragma once

#include <types.h>

#include <EASTL/vector.h>
#include <debug/assert.h>
#include <util/crc.hxx>
#include <util/deferred.hxx>

namespace kuribo {

class SymbolManager {
public:
  SymbolManager() : mEntries(128) { mEntries.resize(0); }
  ~SymbolManager() = default;

  // TODO: We can do better than a singleton
  static SymbolManager& initializeStaticInstance() {
    sInstance = new SymbolManager();
    return *sInstance;
  }
  static SymbolManager& getStaticInstance() {
    KURIBO_ASSERT(sInstance);
    return *sInstance;
  }

  void registerProcedure(eastl::string_view symbol, u32 value) {
    registerProcedure(util::crc32(symbol), value);
  }
  u32 getProcedure(eastl::string_view symbol) {
    return getProcedure(util::crc32(symbol));
  }

  void registerProcedure(u32 symbol, u32 value);
  u32 getProcedure(u32 symbol);

private:
  static SymbolManager* sInstance;

  struct Entry {
    Entry() = default;
    Entry(u32 hash_, u32 value_) : hash(hash_), value(value_) {}

    u32 hash = 0;
    u32 value = 0;
  };
  eastl::vector<Entry> mEntries;
};

KX_EXPORT
void kxRegisterProcedure(const char* symbol, u32 value);

KX_EXPORT
u32 kxGetProcedure(const char* symbol);

} // namespace kuribo