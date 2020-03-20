#pragma once


#include "common.h"

#include <EASTL/string_view.h>

namespace kuribo {
struct KamekLoadParam
{
	eastl::string_view binary;
	mem::Heap* heap = nullptr;
	void** textStartCb = nullptr;
};
enum class KamekLoadResult
{
	OK,
	MalformedRequest,
	InvalidFileType,
	InvalidVersion,
	BadAlloc,
	BadReloc
};

// Note: call __sync after
struct ElfRelocationHandlerParam
{
	typedef void(*Extension)(u32 address);
//	eastl::map<u32, Extension> extensions;
};
bool handleElfRelocation(u8** input, u32 text);
inline void __sync()
{
#ifndef _WIN32
	asm("sync");
	asm("isync");
#endif
}
KamekLoadResult loadKamekBinary(KamekLoadParam param);
} // namespace kuribo
