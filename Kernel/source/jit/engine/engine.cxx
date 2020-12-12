#include "engine.hpp"

#include <algorithm>
#include <array>

namespace gecko_jit {

constexpr std::array<u32, 4> __WiiVIHook{0x7CE33B78, 0x38870034, 0x38A70038,
                                         0x38C7004C};
constexpr std::array<u32, 8> __GcVIHook{0x7C030034, 0x38830020, 0x5485083C,
                                        0x7C7F2A14, 0xA0030000, 0x7C7D2A14,
                                        0x20A4003F, 0xB0030000};

constexpr u32 beginInMemory = 0x80003000;
constexpr u32 endInMemory = 0x81000000;

// Unfortunately we can't use this until added to EASTL or we properly link
// against libc++/libstdc++
#if 0
template<class RandomIt1>
using searcher = std::boyer_moore_horspool_searcher<RandomIt1,
  eastl::hash<typename std::iterator_traits<RandomIt1>::value_type>,
  std::equal_to<>>;

struct DirectHash {
  u32 operator()(u32 p) const {
    return p;
  }
};
#endif

template <typename T> u32* search(u32* begin, u32* end, T search_for) {
#if 0
  std::boyer_moore_horspool_searcher<decltype(search_for.begin()),
    DirectHash,
    std::equal_to<>> searcher(search_for.begin(), search_for.end());
  return std::search(begin, end, searcher);
#endif
  return eastl::search(begin, end, search_for.begin(), search_for.end());
}

u32* FindHookInMemory(GeckoHookType type) {
#ifdef _WIN32
  return nullptr;
#endif

  u32* hooked = nullptr;

  if (type == GeckoHookType::VI_GC) {
    auto found = search((u32*)beginInMemory, (u32*)endInMemory, __GcVIHook);
    if (found == (u32*)endInMemory)
      return nullptr;
    hooked = found;

    // TODO
  } else if (type == GeckoHookType::VI_WII) {
    const auto found =
        search((u32*)beginInMemory, (u32*)endInMemory, __WiiVIHook);
    if (found == (u32*)endInMemory)
      return nullptr;
    hooked = found;

    return hooked + 14;
  }
  return nullptr;
  // auto found = std::find(hooked, hooked + 32, 0x4BE46D88);
  //
  // return found == hooked + 32 ? nullptr : found;
}

} // namespace gecko_jit

namespace kuribo {

kxGeckoJitEngine* kxCreateJitEngine(u8* memory_begin, u32 memory_size) {
  return new gecko_jit::JITEngine(memory_begin, memory_size);
}
void kxDestroyJitEngine(kxGeckoJitEngine* pEngine) { delete pEngine; }

} // namespace kuribo