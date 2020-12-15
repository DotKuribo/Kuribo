#include "FreeListAllocator.h"
#include "Utils.h"           /* CalculatePaddingWithHeader */
#include <EASTL/algorithm.h> // std::max
#include <limits>            /* limits_max */
#include <stdlib.h>          /* malloc, free */

#include "config.h"
#include "debug.h"
#include "types.h"

#ifdef _WIN32
#include <stdio.h>
#endif

namespace xalloc {

void* FreeListAllocator::Allocate(const std::size_t size,
                                  const std::size_t alignment) {
  const std::size_t allocationHeaderSize =
      sizeof(FreeListAllocator::AllocationHeader);
  const std::size_t freeHeaderSize = sizeof(FreeListAllocator::FreeHeader);
  KURIBO_ASSERT("Allocation size must be bigger" && size >= sizeof(Node));
  KURIBO_ASSERT("Alignment must be 8 at least" && alignment >= 8);

  // Search through the free list for a free block that has enough space to
  // allocate our data
  std::size_t padding;
  Node *affectedNode, *previousNode;
  this->Find(size, alignment, padding, previousNode, affectedNode);
  if (affectedNode == nullptr) {
    KURIBO_PRINTF("Allocation failed\n");
    KURIBO_PRINTF("Looking for %u bytes\n", size);
    PrintStats();
  }
  KURIBO_ASSERT(affectedNode != nullptr && "Not enough memory");

  const std::size_t alignmentPadding = padding - allocationHeaderSize;
  const std::size_t requiredSize = size + padding;

  const std::size_t rest = affectedNode->data.blockSize - requiredSize;

  if (rest > 0) {
    // We have to split the block into the data block and a free block of size
    // 'rest'
    Node* newFreeNode = (Node*)((std::size_t)affectedNode + requiredSize);
    newFreeNode->data.blockSize = rest;
    m_freeList.insert(affectedNode, newFreeNode);
  }
  m_freeList.remove(previousNode, affectedNode);

  // Setup data block
  const std::size_t headerAddress =
      (std::size_t)affectedNode + alignmentPadding;
  const std::size_t dataAddress = headerAddress + allocationHeaderSize;
  ((FreeListAllocator::AllocationHeader*)headerAddress)->blockSize =
      requiredSize;
  ((FreeListAllocator::AllocationHeader*)headerAddress)->padding =
      alignmentPadding;
  ((FreeListAllocator::AllocationHeader*)headerAddress)->heap = this;

  m_used += requiredSize;
  m_peak = eastl::max(m_peak, m_used);

#ifdef _DEBUG
  // std::cout << "A" << "\t@H " << (void*)headerAddress << "\tD@ " <<
  // (void*)dataAddress << "\tS " << ((FreeListAllocator::AllocationHeader*)
  // headerAddress)->blockSize << "\tAP " << alignmentPadding << "\tP " <<
  // padding << "\tM " << m_used << "\tR " << rest << std::endl;
#endif

  return (void*)dataAddress;
}

void FreeListAllocator::Find(const std::size_t size,
                             const std::size_t alignment, std::size_t& padding,
                             Node*& previousNode, Node*& foundNode) {
  switch (m_pPolicy) {
  case PlacementPolicy::FIND_FIRST:
    FindFirst(size, alignment, padding, previousNode, foundNode);
    break;
  case PlacementPolicy::FIND_BEST:
    FindBest(size, alignment, padding, previousNode, foundNode);
    break;
  }
}

void FreeListAllocator::FindFirst(const std::size_t size,
                                  const std::size_t alignment,
                                  std::size_t& padding, Node*& previousNode,
                                  Node*& foundNode) {
  // Iterate list and return the first free block with a size >= than given size
  Node *it = m_freeList.head, *itPrev = nullptr;

  while (it != nullptr) {
    padding = Utils::CalculatePaddingWithHeader(
        (std::size_t)it, alignment,
        sizeof(FreeListAllocator::AllocationHeader));
    const std::size_t requiredSpace = size + padding;
    if (it->data.blockSize >= requiredSpace) {
      break;
    }
    itPrev = it;
    it = it->next;
  }
  previousNode = itPrev;
  foundNode = it;
}

void FreeListAllocator::FindBest(const std::size_t size,
                                 const std::size_t alignment,
                                 std::size_t& padding, Node*& previousNode,
                                 Node*& foundNode) {
  // Iterate WHOLE list keeping a pointer to the best fit
  std::size_t smallestDiff = std::numeric_limits<std::size_t>::max();
  Node* bestBlock = nullptr;
  Node *it = m_freeList.head, *itPrev = nullptr;
  while (it != nullptr) {
    padding = Utils::CalculatePaddingWithHeader(
        (std::size_t)it, alignment,
        sizeof(FreeListAllocator::AllocationHeader));
    const std::size_t requiredSpace = size + padding;
    if (it->data.blockSize >= requiredSpace &&
        (it->data.blockSize - requiredSpace < smallestDiff)) {
      bestBlock = it;
    }
    itPrev = it;
    it = it->next;
  }
  previousNode = itPrev;
  foundNode = bestBlock;
}

void FreeListAllocator::Free(void* ptr) {
  // Insert it in a sorted position by the address number
  const std::size_t currentAddress = (std::size_t)ptr;
  const std::size_t headerAddress =
      currentAddress - sizeof(FreeListAllocator::AllocationHeader);
  const FreeListAllocator::AllocationHeader* allocationHeader{
      (FreeListAllocator::AllocationHeader*)headerAddress};

  if (allocationHeader->blockSize > (u16)-1) {
    KURIBO_LOG("Invalid free call.\n");
    return;
  }

  Node* freeNode = (Node*)(headerAddress);
  freeNode->data.blockSize =
      allocationHeader->blockSize + allocationHeader->padding;
  freeNode->next = nullptr;

  Node* it = m_freeList.head;
  Node* itPrev = nullptr;
  while (it != nullptr) {
    if (ptr < it) {
      m_freeList.insert(itPrev, freeNode);
      break;
    }
    itPrev = it;
    it = it->next;
  }

  m_used -= freeNode->data.blockSize;

  // Merge contiguous nodes
  Coalescence(itPrev, freeNode);

#ifdef _DEBUG
  // std::cout << "F" << "\t@ptr " << ptr << "\tH@ " << (void*)freeNode << "\tS
  // " << freeNode->data.blockSize << "\tM " << m_used << std::endl;
#endif
}

void FreeListAllocator::Coalescence(Node* previousNode, Node* freeNode) {
  if (freeNode->next != nullptr &&
      (std::size_t)freeNode + freeNode->data.blockSize ==
          (std::size_t)freeNode->next) {
    freeNode->data.blockSize += freeNode->next->data.blockSize;
    m_freeList.remove(freeNode, freeNode->next);
#ifdef _DEBUG
    // std::cout << "\tMerging(n) " << (void*)freeNode << " & " <<
    // (void*)freeNode->next << "\tS " << freeNode->data.blockSize << std::endl;
#endif
  }

  if (previousNode != nullptr &&
      (std::size_t)previousNode + previousNode->data.blockSize ==
          (std::size_t)freeNode) {
    previousNode->data.blockSize += freeNode->data.blockSize;
    m_freeList.remove(previousNode, freeNode);
#ifdef _DEBUG
    // std::cout << "\tMerging(p) " << (void*)previousNode << " & " <<
    // (void*)freeNode << "\tS " << previousNode->data.blockSize << std::endl;
#endif
  }
}

void FreeListAllocator::Reset() {
  m_used = 0;
  m_peak = 0;
  Node* firstNode = (Node*)getData();
  // TODO: Should this subtract the size of a node?
  firstNode->data.blockSize = m_totalSize;
  firstNode->next = nullptr;
  m_freeList.head = nullptr;
  m_freeList.insert(nullptr, firstNode);
}

bool FreeListAllocator::AddFreeRegion(void* begin, void* end) {
  const auto size = static_cast<char*>(end) - static_cast<char*>(begin);

  if (size <= sizeof(Node)) {
    // Don't bother: size is too small
    KURIBO_LOG("Couldn't add free region: %u bytes is too small.\n",
               static_cast<u32>(size));
    return false;
  }
  PrintStats();
  Node* pNode = reinterpret_cast<Node*>(begin);
  // TODO: Is this subtraction necessary?
  pNode->data.blockSize = size - sizeof(Node);
  pNode->next = m_freeList.head;
  m_freeList.insert(nullptr, pNode);
  PrintStats();
  return true;
}

void FreeListAllocator::PrintStats() const {
  KURIBO_PRINTF("-------\n");
  for (const Node* it = m_freeList.head; it != nullptr; it = it->next) {
    KURIBO_PRINTF("Free block: Size %u\n",
               static_cast<u32>(it->data.blockSize));
  }
  KURIBO_PRINTF("-------\n");
}

} // namespace xalloc