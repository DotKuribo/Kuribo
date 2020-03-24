#include <stdint.h>
#include "modules/comet/file_format.hxx"

void comet_app_install(void* image_start, void* vaddr_load, uint32_t load_size);

extern "C" int main(int image);

#ifdef KURIBO_PLATFORM_WII
extern "C" void _start(int image);
void _start(int image) { main(image); }

extern "C" void __eabi() {}
#endif

int main(int image)
{
#ifdef _WIN32
	comet_app_install(nullptr, 0, 0);
#else
	comet::CmxHeader* header = reinterpret_cast<comet::CmxHeader*>(image);
	// comet_app_install((void*)image, (void*)header->vaddr_load, header->load_size);
	comet_app_install(0, 0, 0);
#endif
	return 0;
}