#pragma once

#include <EASTL/unique_ptr.h>

#include "config.h"
#include "types.h"
#include "debug.h"
#include "debug/log.h"

#include "util/deferred.hxx"
#include "core/sync.hxx"


namespace kuribo {

using AbortHandler = void(*)(const char* reason);

void DefaultAbortHandler(const char* reason);

class System final
{
	friend class DeferredSingleton<System>;
public:
	static DeferredSingleton<System> sInstance;
    static System& getSystem() { KURIBO_ASSERT(sInstance.isStaticInstanceInitialized()); return sInstance.getInstance(); }
    static bool createSystem() { return sInstance.initializeStaticInstance(); }
    static bool destroySystem() { return sInstance.deinitializeStaticInstance(); }

    inline void abort(const char* reason)
	{
		KURIBO_LOG("---\nkuribo ALERT: ABORTING SYSTEM\n---\n");
		KURIBO_LOG("USER REASON: %s\n", reason);
		KURIBO_LOG("UPTIME: %u SECONDS\n", 0);
		// TODO -- print stack
		mAbortHandler(reason);
	}
	void setAbortHandler(AbortHandler handler) { mAbortHandler = handler; }

private:
	AbortHandler mAbortHandler = DefaultAbortHandler;
public:
    System() = default;
    ~System() = default;
};


} // namespace kuribo