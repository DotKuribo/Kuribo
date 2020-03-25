#include "config.h"
#include "log.h"
#include "assert.h"

int ScopedLog::sLogIndent;

#if KURIBO_PLATFORM == KURIBO_PL_TYPE_WII || KURIBO_PLATFORM == KURIBO_PL_TYPE_GC
#define getSp()   ({volatile  u32 _rval; \
            asm volatile("mr %0,%%r1" : "=r"(_rval)); _rval;})
namespace {
void __stacktrace(const u32* sp) {
	KURIBO_LOG_FUNCTION("Stack trace:\n");

	auto isStackAddr = [](u32 v) {
		if (v == 0 || v == static_cast<u32>(-1)) return false;
		switch (v & 0x30000000) {
		case 0x00000000: // MEM1
			return (v & 0x0FFFFFFF) < 0x1800000; // Retail (24MB)
		case 0x10000000: // MEM2
			return (v & 0x0FFFFFFF) < 0x4000000; // Retail (64MB)
		default:
			return false;
		}
	};

	KURIBO_LOG_FUNCTION("Address:   BackChain   LR save\n");
	for (int i = 0; i < 16; ++i) {
		if (!isStackAddr(*sp)) break;
		KURIBO_LOG_FUNCTION("%08X:  %08X    %08X\n", sp, sp[0], sp[1]);
		sp = reinterpret_cast<const u32*>(sp[0]);
	}
}
}
#endif

extern "C"
int kuribo_fail_assertion(const char* file, int line, const char* exp, const char* msg, enum kuribo_assert_type type) {
    KURIBO_LOG("Failed assert: %s\n", exp ? exp : "");

	if (msg) KURIBO_LOG("%s\n", msg);

	if (file)
	{
		KURIBO_LOG("[File] %s\n", file);

		if (line >= 0) KURIBO_LOG("[Line] %i\n", line);
	}

#ifdef KURIBO_PLATFORM_WII
	__stacktrace(reinterpret_cast<const u32*>(getSp()));

	// GameIO::OSHalt("Failed assertion");
	while (1); // TODO -- Use System
	// GameIO::OSFatal(exp ? exp : msg ? msg : "Failed assertion");
#else
	throw "";
#endif

	return 0;
}

