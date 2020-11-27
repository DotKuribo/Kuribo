#include "SymbolManager.hxx"

namespace kuribo {

DeferredInitialization<SymbolManager> SymbolManager::sInstance;

void SymbolManager::registerProcedure(u32 symbol, u32 value) {
  mEntries.emplace_back(symbol, value);
}
u32 SymbolManager::getProcedure(u32 symbol) {
  const auto find_proc = [symbol](const Entry& entry) {
    return entry.hash == symbol;
  };
  const auto found =
      eastl::find_if(mEntries.begin(), mEntries.end(), find_proc);
  if (found == mEntries.end())
    return 0;
  return found->value;
}

void kxRegisterProcedure(const char* symbol, u32 value) {
  SymbolManager::getStaticInstance().registerProcedure(symbol, value);
}
u32 kxGetProcedure(const char* symbol) {
  return SymbolManager::getStaticInstance().getProcedure(symbol);
}

} // namespace kuribo