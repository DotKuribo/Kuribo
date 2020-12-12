#pragma once

#include "config.h"
#include "debug.h"
#include "memory/heap.hxx"
#include "memory/stack_alloc.hxx"
#include "types.h"

namespace kuribo {
namespace mem {

enum class GlobalHeapType {
  Default, // Decide between MEM1 and MEM2
  MEM1,
  MEM2=MEM1
};

void Init(char* mem1b, u32 mem1s, char* mem2b, u32 mem2s);
bool AddRegion(void* start, u32 size, bool mem2);
Heap& GetHeap(GlobalHeapType type);

inline Heap& GetDefaultHeap() { return GetHeap(GlobalHeapType::Default); }

void* Alloc(u32 size, Heap& heap, u32 align = 8) noexcept;
void Free(void* ptr, Heap& heap);
void Free(void* ptr);
} // namespace mem

} // namespace kuribo

inline void* operator new(u32 size, kuribo::mem::Heap* heap,
                          u32 align = 8) noexcept {
  return kuribo::mem::Alloc(
      size,
      heap ? *heap : kuribo::mem::GetHeap(kuribo::mem::GlobalHeapType::Default),
      align);
}

inline void* operator new[](u32 size, kuribo::mem::Heap* heap,
                            u32 align = 8) noexcept {
  return operator new(size, heap, align);
}
void* operator new(u32 size);
#include <EASTL/unique_ptr.h>

namespace kuribo {
template <typename T, typename... Args>
inline eastl::unique_ptr<T> make_unique(kuribo::mem::Heap& heap, u32 align = 8,
                                        Args... args) noexcept {
  return eastl::unique_ptr<T>(new (&heap, align) T(args...));
};

template <typename T>
inline eastl::unique_ptr<T> make_unique(kuribo::mem::Heap& heap,
                                        u32 align = 8) noexcept {
  return eastl::unique_ptr<T>(new (&heap, align) T());
};
} // namespace kuribo
void _free(void* p);
void* _malloc(u32 s);
void* _realloc(void* ptr, u32 size);

namespace kuribo::mem {

template <GlobalHeapType H> class Allocator {
public:
  inline Allocator(const char* = nullptr) : mHeap(GetHeap(H)) {}

  //	inline Allocator(const Allocator&)
  //	{}
  //	inline Allocator(const Allocator&, const char*)
  //	{}
  //	inline Allocator& operator=(const Allocator&)
  //	{}

  inline bool operator==(const Allocator& other) {
    return &mHeap == &other.mHeap;
  }

  inline bool operator!=(const Allocator& other) { return !operator==(other); }

  inline void* allocate(size_t n, int /*flags*/ = 0) { return mHeap.alloc(n); }

  inline void* allocate(size_t n, size_t alignment, size_t alignmentOffset,
                        int /*flags*/ = 0) {
    KURIBO_ASSERT(alignment == alignmentOffset);
    return mHeap.alloc(n, alignment);
  }

  inline void deallocate(void* p, size_t /*n*/) { return mHeap.free(p); }

  inline const char* get_name() const { return "Allocator"; }

  inline void set_name(const char*) {}

  Heap& mHeap;
};

} // namespace kuribo::mem
