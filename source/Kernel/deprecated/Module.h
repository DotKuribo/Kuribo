/*
 * @file
 * @brief	Module interoperability headers.
                        Enabled with KURIBO_MODULE
                        If you wish to use the kuribo_module_impl* functions,
 define KURIBO_MODULE_IMPL in one source file.
*/

#pragma once

#include <sdk/kuribo_sdk.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef KURIBO_MODULE
void kuribo_module_impl_setup_memory(kuribo_module_context* context);
void* kuribo_module_impl_alloc(uint32_t size, uint32_t align);
void kuribo_module_impl_free(void* block);
#endif

#ifdef __cplusplus
}
#endif

#ifdef KURIBO_MODULE
#ifdef KURIBO_MODULE_IMPL
static kuribo_module_context* gInterop;

void kuribo_module_impl_setup_memory(kuribo_module_context* context) {
  gInterop = context;
}
void* kuribo_module_impl_alloc(uint32_t size, uint32_t align) {
  return gInterop->kuribo_alloc(gInterop->Token, size, align);
}
void kuribo_module_impl_free(void* block) {
  return gInterop->kuribo_free(gInterop->Token, block);
}
#endif

#ifdef __cplusplus
inline void* operator new(uint32_t size) {
  return kuribo_module_impl_alloc(size, 8);
}
inline void* operator new(uint32_t size, uint32_t align) {
  return kuribo_module_impl_alloc(size, align);
}
inline void* operator new[](uint32_t size) { return operator new(size); }
inline void* operator new[](uint32_t size, uint32_t align) {
  return operator new(size, align);
}
inline void operator delete(void* p) { kuribo_module_impl_free(p); }
inline void operator delete[](void* p) { kuribo_module_impl_free(p); }
#endif
#endif