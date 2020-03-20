#include "config.h"
#include "debug.h"
#include "system/memory.hxx"
#include "free_list_heap.hxx"
#include "util/deferred.hxx"


namespace kuribo {
	
namespace mem {
	void* Alloc(u32 size, Heap& heap, u32 align) noexcept { return heap.alloc(size > 8 ? size : 8, align); }

	void Free(void* ptr, Heap& heap) { heap.free(ptr); }
	void Free(void* ptr) { /* TODO */ }

	DeferredInitialization<FreeListHeap> sMem1Heap, sMem2Heap;

	void Init(char* mem1b, u32 mem1s, char* mem2b, u32 mem2s) {
		sMem1Heap.initialize(mem1b, mem1s);
		sMem2Heap.initialize(mem2b, mem2s);
	}

	void AddRegion(u32 start, u32 size, bool mem2) {

	}

	Heap& GetHeap(GlobalHeapType type) {
		switch (type) {
		case GlobalHeapType::Default:
		case GlobalHeapType::MEM1:
			return sMem1Heap;
		case GlobalHeapType::MEM2:
			return sMem2Heap;
		}
	}

}

}

void* _malloc(u32 s)
{
	return kuribo::mem::Alloc(s, kuribo::mem::GetDefaultHeap());
}
void* _malloc_ex(u32 s, u32 a)
{
	return kuribo::mem::Alloc(s, kuribo::mem::GetDefaultHeap(), a);
}

void _free(void* p)
{
	if (p)
		kuribo::mem::Free(p);
}

void* operator new (u32 size)
{
	return kuribo::mem::Alloc(size, kuribo::mem::GetDefaultHeap());
}
void* operator new[](u32 size)
{
	return operator new(size);
}

void operator delete(void* p)
{
	if (p) kuribo::mem::Free(p);
}
void operator delete[](void* p)
{
	if (p) kuribo::mem::Free(p);
}

#if __cplusplus >= 201402L
void operator delete(void* ptr, u32)
{
	operator delete(ptr);
}
void operator delete[](void* ptr, u32)
{
	operator delete(ptr);
}
#endif

// EASTL

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return kuribo::mem::Alloc(size, kuribo::mem::GetDefaultHeap(), 8);
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	KURIBO_ASSERT(alignment == alignmentOffset);
	return kuribo::mem::Alloc(size, kuribo::mem::GetDefaultHeap(), alignment);
}
