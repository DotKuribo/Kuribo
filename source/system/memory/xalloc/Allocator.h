#pragma once

#include <cstddef> // size_t
#include <EASTL/unique_ptr.h>

class Allocator {
protected:
    std::size_t m_totalSize;
    std::size_t m_used = 0;
    std::size_t m_peak = 0;
	char* getData() { return mExtData ? mExtData : mData.get(); }
private:
	eastl::unique_ptr<char[]> mData = nullptr;
	char* mExtData = nullptr;
public:
    Allocator(eastl::unique_ptr<char []> data, std::size_t totalSize)
		: m_totalSize(totalSize), mData(std::move(data))
	{}
	// Non-owning reference
	Allocator(char& data, std::size_t size)
		: m_totalSize(size), mExtData(&data)
	{}

    virtual ~Allocator() = default;

    virtual void* Allocate(const std::size_t size, const std::size_t alignment = 0) = 0;
    virtual void Free(void* ptr) = 0;
	virtual void Reset() = 0;

    friend class Benchmark;
};
