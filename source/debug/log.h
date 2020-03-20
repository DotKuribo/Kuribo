#pragma once

#include "config.h"
#include "types.h"

#ifdef KURIBO_ENABLE_LOG
    #if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII || KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
        // TODO
        #define KURIBO_LOG_FUNCTION(...)
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
#endif

#ifdef __cplusplus
struct ScopedLog {
#ifdef KURIBO_ENABLE_LOG
	ScopedLog(const char* msg, const char* file = nullptr, int line = -1, const char* fn = nullptr)
	{
		char ibuf[20]{};
		indent(ibuf, 20);
		KURIBO_LOG_FUNCTION("\n%s@%i in %s: %s%s {\n", file ? file : "", line, fn, ibuf, msg);
		++sLogIndent;
		
	}
	~ScopedLog()
	{
		--sLogIndent;
		char ibuf[20]{};
		indent(ibuf, 20);
		KURIBO_LOG_FUNCTION("%s}\n", ibuf);
	}
private:
	void indent(char* out, u32 size)
	{
		for (int i = 0; i < sLogIndent; ++i)
			out[i] = '\t';
	}
#endif
	static int sLogIndent;

};
#endif

#ifdef KURIBO_ENABLE_LOG
#define KURIBO_SCOPED_LOG(msg) ScopedLog _(msg, __FILE__, __LINE__, __FUNCTION__)
#else
#define KURIBO_SCOPED_LOG(...)
#endif