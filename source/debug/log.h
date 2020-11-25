#pragma once

#include "config.h"
#include "types.h"

#define mftbl()   ({volatile  u32 _rval; \
            asm volatile("mftbl %0" : "=r" (_rval)); _rval;})

inline u32 get_tick() {
#ifdef _WIN32
  return 0;
#else
  return mftbl();
#endif
}

// courtesy of bslug
#define TB_BUS_CLOCK				243000000u
#define TB_CORE_CLOCK				729000000u
#define TB_TIMER_CLOCK				(TB_BUS_CLOCK/4000)	

#define ticks_to_secs(ticks)		(((ticks)/(TB_TIMER_CLOCK*1000)))
#define ticks_to_millisecs(ticks)	(((ticks)/(TB_TIMER_CLOCK)))
#define ticks_to_microsecs(ticks)	((((ticks)*8)/(TB_TIMER_CLOCK/125)))
#define ticks_to_nanosecs(ticks)	((((ticks)*8000)/(TB_TIMER_CLOCK/125)))

#define secs_to_ticks(sec)			((sec)*(TB_TIMER_CLOCK*1000))
#define millisecs_to_ticks(msec)	((msec)*(TB_TIMER_CLOCK))
#define microsecs_to_ticks(usec)	(((usec)*(TB_TIMER_CLOCK/125))/8)
#define nanosecs_to_ticks(nsec)		(((nsec)*(TB_TIMER_CLOCK/125))/8000)

#ifdef KURIBO_ENABLE_LOG
#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII || KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
#include "api/HostInterop.h"
#define __OSReport(...) do { \
			( (void (*) (const char*, ...)) FFI_NAME(os_report)) (__VA_ARGS__); \
			} while (0)
#define KURIBO_LOG_FUNCTION __OSReport
#else
#include <stdio.h>
#define KURIBO_LOG_FUNCTION printf
#endif

#define STRINGIZE(x) STRINGIZE_(x)
#define STRINGIZE_(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define KURIBO_LOG(...) KURIBO_LOG_FUNCTION("[" __FILE__ ":" LINE_STRING "] " __VA_ARGS__ )
#else
#define KURIBO_LOG(...)
#define KURIBO_LOG_FUNCTION(...)
#endif

#ifdef __cplusplus
struct ScopedLog {
#ifdef KURIBO_ENABLE_LOG
  ScopedLog(const char* msg, const char* file = nullptr, int line = -1, const char* fn = nullptr)
    : m_msg(msg)
  {
    char ibuf[20]{};
    ibuf[0] = ' ';
    // indent(ibuf, 20);
    KURIBO_LOG_FUNCTION("\n<%s:%i in %s: %s%s> {\n", file ? file : "", line, fn, &ibuf, msg);
    ++sLogIndent;
    m_begin = get_tick();
  }
  ~ScopedLog()
  {
    u32 end = get_tick();
    u32 time = end - m_begin;
    const u32 ps = ticks_to_microsecs(time);
    const u32 ns = ticks_to_nanosecs(time);

    --sLogIndent;
    char ibuf[20]{};
    ibuf[0] = ' ';
    // indent(ibuf, 20);
    KURIBO_LOG_FUNCTION("%s} </%s> (%u ticks, %u microseconds, %u nanoseconds)\n", &ibuf, m_msg, time, ps, ns);
  }
private:
  void indent(char* out, u32 size)
  {
    for (int i = 0; i < sLogIndent; ++i)
      out[i] = '\t';
  }
  const char* m_msg;
  u32 m_begin;
#endif
  static int sLogIndent;
};
#endif

#ifdef KURIBO_ENABLE_LOG
#define KURIBO_SCOPED_LOG(msg) ScopedLog _(msg, __FILE__, __LINE__, __FUNCTION__)
#else
#define KURIBO_SCOPED_LOG(...)
#endif