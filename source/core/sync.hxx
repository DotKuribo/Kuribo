#pragma once

#include "config.h"
#include "debug.h"
#include "types.h"

#if KURIBO_PLATFORM != KURIBO_PL_TYPE_WII &&                                   \
    KURIBO_PLATFORM != KURIBO_PL_TYPE_GC
#include <mutex>
#endif

namespace kuribo {

#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII ||                                   \
    KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
#define mfmsr()                                                                \
  ({                                                                           \
    volatile u32 _rval;                                                        \
    asm volatile("mfmsr %0" : "=r"(_rval));                                    \
    _rval;                                                                     \
  })
#define mtmsr(val) asm volatile("mtmsr %0" : : "r"(val))
inline u32 getMachineState() noexcept { return mfmsr(); }
inline void setMachineState(int state) noexcept { mtmsr(state); }
#else
#include <mutex>
#endif

inline bool setInterrupts(bool state) noexcept {
#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII ||                                   \
    KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
  u32 last = getMachineState();
  setMachineState(state ? (last | 0x8000) : (last & ~0x8000));
  return last & 0x8000;
#else
  return false;
#endif
}

//  template<bool withSched=false>
struct Critical {
#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII ||                                   \
    KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
  Critical() noexcept {
    restore = setInterrupts(false);
    //    if (withSched)
    //        GameIO::SetScheduler(false);
  }
  ~Critical() noexcept {
    setInterrupts(restore);
    //    if (withSched)
    //        GameIO::SetScheduler(true);
  }
#endif
  bool restore = false;
};
struct CriticalContext {
#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII ||                                   \
    KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
  void lock() noexcept { restore = setInterrupts(false); }
  void unlock() noexcept { setInterrupts(restore); }
#endif
  bool restore = false;
};
#if 0
#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII ||                                   \
    KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
struct WiiMutex {
	void lock() noexcept {
		GameIO::OSLockMutex(low);
	}
	void unlock() noexcept {
		GameIO::OSUnlockMutex(low);
	}
	WiiMutex() noexcept {
		GameIO::OSInitMutex(low);
	}

private:
	GameIO::OSMutex low;
};
using Mutex = WiiMutex;
#else
using Mutex = std::mutex;
#endif
#endif

} // namespace kuribo