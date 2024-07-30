#pragma once

#include <types.h>

#include <EASTL/vector.h>
#include <debug/assert.h>
#include <util/crc.hxx>
#include <util/deferred.hxx>

#include <memory/heap.hxx>

namespace kuribo {

class SymbolManager {
public:
  SymbolManager(mem::Heap& heap) : mpHeap(&heap), mEntryCapacity(128) {
    mEntries = new (heap) Entry[mEntryCapacity];
  }
  ~SymbolManager() { mpHeap->free(mEntries); }

  // TODO: We can do better than a singleton
  static SymbolManager& initializeStaticInstance(mem::Heap& heap) {
    sInstance = new (heap) SymbolManager(heap);
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

  mem::Heap* mpHeap;
  Entry* mEntries;
  int mEntrySize = 0;
  int mEntryCapacity;
};

KX_EXPORT
void kxRegisterProcedure(const char* symbol, u32 value);

KX_EXPORT
u32 kxGetProcedure(const char* symbol);

KX_EXPORT
void kxRegisterProcedureEx(const char* symbol_str, u32 symbol_strlen, u32 value,
                           u32 crc32_hint);

KX_EXPORT
u32 kxGetProcedureEx(const char* symbol_str, u32 symbol_strlen, u32 crc32_hint);

} // namespace kuribo