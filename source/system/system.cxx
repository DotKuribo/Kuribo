#include "system.hxx"

namespace kuribo {

void DefaultAbortHandler(const char* reason)
{
	Critical g;

	while (1) {}
}


DeferredSingleton<System> System::sInstance;


} // namespace kuribo
