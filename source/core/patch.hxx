#pragma once

#include "config.h"
#include "types.h"
#include "debug/log.h"

namespace kuribo {

#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII || KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
#define dcbst(_val)  asm volatile("dcbst 0, %0" : : "r" (_val))
#define dcbf(_val)  asm volatile("dcbf 0, %0" : : "r" (_val))
#define icbi(_val)  asm volatile("icbi 0, %0" : : "r" (_val))
static inline void flushAddr(void* addr)
{
	dcbf(addr);
	icbi(addr);
}
#else
inline void flushAddr(void* addr) {}
#endif

//! @brief Write a value at a specified address
//!
//! @param[in] addr  Address to write the pointer at.
//! @param[in] value Value to write.
//!
template<typename T>
static inline void directWrite(T* dst, T val)
{
	static_assert(sizeof(T) <= 32, "Datablock restrictions.");

	KURIBO_LOG("%u-bit write to %p (value: %08x)\n", sizeof(T) * 8, dst, val);

#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII || KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
	*dst = val;
	flushAddr(dst);
#endif
}

//! @brief Write a branch (b) or call (bl) instruction at a specified address.
//!
//! @param[in] addr Address to write the pointer at (branch start).
//! @param[in] ptr  Where to branch to (branch end).
//! @param[in] lk	If the LR should be set. (bl rather than b)
//!
static inline void directBranchEx(void* addr, void* ptr, bool lk = false)
{
  const u32 delta = reinterpret_cast<u32>(ptr) - reinterpret_cast<u32>(addr);
  if (delta & 0b11)
    KURIBO_LOG("Address translation is too granular:"
               " 0x%08X will be clipped to 0x%08X\n",
               delta, delta & ~0b11);
  directWrite(reinterpret_cast<u32*>(addr),
              (delta & 0x3ffffff & ~0b11) | 0x48000000 | !!lk);
}

//! @brief Write a branch (b) instruction at a specified address.
//!
//! @param[in] addr Address to write the pointer at (branch start).
//! @param[in] ptr  Where to branch to (branch end).
//!
static inline void directBranch(void* addr, void* ptr)
{
	directBranchEx(addr, ptr, false);
}

//! @brief Write a branch with link (bl) instruction at a specified address.
//!
//! @param[in] addr Address to write the pointer at (branch start).
//! @param[in] ptr  Where to branch to (branch end).
//!
static void directCall(void* addr, void* ptr)
{
	directBranchEx(addr, ptr, true);
}


} // namespace kuribo