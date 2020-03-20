#pragma once

#include "Allocator.h"
#include "SinglyLinkedList.h"

namespace xalloc {

class FreeListAllocator final : public Allocator {
public:
	enum class PlacementPolicy {
		FIND_FIRST,
		FIND_BEST
	};

private:
	struct FreeHeader {
		std::size_t blockSize;
	};
	struct AllocationHeader {

		Allocator* heap;
		unsigned short blockSize;
		char padding; // char
		char _;
	};

	using Node = SinglyLinkedList<FreeHeader>::Node;

	PlacementPolicy m_pPolicy;
	SinglyLinkedList<FreeHeader> m_freeList;

public:
	FreeListAllocator(eastl::unique_ptr<char[]> data, std::size_t size, const PlacementPolicy pPolicy)
		: Allocator(eastl::move(data), size), m_pPolicy(pPolicy)
	{
		Reset();
	}
	FreeListAllocator(char& data, std::size_t size, const PlacementPolicy p)
		: Allocator(data, size), m_pPolicy(p)
	{
		Reset();
	}

	void* Allocate(const std::size_t size, const std::size_t alignment = 0) override;

	void Free(void* ptr) override;

	void Reset() override;
private:
	FreeListAllocator(FreeListAllocator& freeListAllocator);

	void Coalescence(Node* prevBlock, Node* freeBlock);

	void Find(const std::size_t size, const std::size_t alignment, std::size_t& padding, Node*& previousNode, Node*& foundNode);
	void FindBest(const std::size_t size, const std::size_t alignment, std::size_t& padding, Node*& previousNode, Node*& foundNode);
	void FindFirst(const std::size_t size, const std::size_t alignment, std::size_t& padding, Node*& previousNode, Node*& foundNode);
};

} // namespace xalloc