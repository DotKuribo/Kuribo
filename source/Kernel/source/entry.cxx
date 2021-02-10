#include <stdint.h>

void comet_app_install(void* image_start, void* vaddr_load, uint32_t load_size);

extern "C" int main(int image);

#ifdef KURIBO_PLATFORM_WII
extern "C" void kxStart(int image);
__attribute__((section(".kx_main"))) void kxStart(int image) { main(image); }

extern "C" void __eabi() {}
#endif

int main(int image) {
  comet_app_install(nullptr, 0, 0);
  return 0;
}