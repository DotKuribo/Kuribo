#include "SymbolManager.hxx"
// #include "Common/debug.h"

namespace kuribo {

SymbolManager* SymbolManager::sInstance;

void SymbolManager::registerProcedure(u32 symbol, u32 value) {
  mEntries[mEntrySize++] = Entry{symbol, value};
}
u32 SymbolManager::getProcedure(u32 symbol) {
  const auto find_proc = [symbol](const Entry& entry) {
    // KURIBO_LOG("Sym: %08x %08x\n", entry.hash, entry.value);
    return entry.hash == symbol;
  };
  const auto found = eastl::find_if(mEntries, mEntries + mEntrySize, find_proc);
  if (found == mEntries + mEntrySize) {
    // KURIBO_LOG("Cannot find symbol\n");
    return 0;
  }
  return found->value;
}

void kxRegisterProcedure(const char* symbol, u32 value) {
  // KURIBO_LOG("Registering %s at %p\n", symbol, (void*)value);
  SymbolManager::getStaticInstance().registerProcedure(symbol, value);
}
u32 kxGetProcedure(const char* symbol) {
  return SymbolManager::getStaticInstance().getProcedure(symbol);
}

void kxRegisterProcedureEx(const char* symbol_str, u32 symbol_strlen, u32 value,
                           u32 crc32_hint) {
  if (crc32_hint == 0) {
    eastl::string_view sv(symbol_str, symbol_strlen);
    crc32_hint = util::crc32(sv);
  }
  SymbolManager::getStaticInstance().registerProcedure(crc32_hint, value);
}

u32 kxGetProcedureEx(const char* symbol_str, u32 symbol_strlen,
                     u32 crc32_hint) {
  if (crc32_hint == 0) {
    eastl::string_view sv(symbol_str, symbol_strlen);
    crc32_hint = util::crc32(sv);
  }
  return SymbolManager::getStaticInstance().getProcedure(crc32_hint);
}

} // namespace kuribo