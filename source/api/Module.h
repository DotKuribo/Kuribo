/*
 * @file
 * @brief	Module interoperability headers.
			Enabled with KURIBO_MODULE
			If you wish to use the kuribo_module_impl* functions, define KURIBO_MODULE_IMPL in one source file.
*/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum kuribo_module_call
{
	KURIBO_MODULE_CALL_ATTACH,
	KURIBO_MODULE_CALL_DETACH
};

/*
Used to keep track of memory allocations of modules.
*/
struct kuribo_token;

struct kuribo_module_context
{
	uint32_t id;

	void* (*kuribo_alloc)(
		struct kuribo_token* token,
		uint32_t size,
		uint32_t align);
	void (*kuribo_free)(struct kuribo_token* token, void* block);

	struct kuribo_token* Token;
};

typedef void* (*kuribo_module_prologue)(enum kuribo_module_call, struct kuribo_module_context*);

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

void kuribo_module_impl_setup_memory(kuribo_module_context* context)
{
	gInterop = context;
}
void* kuribo_module_impl_alloc(uint32_t size, uint32_t align)
{
	return gInterop->kuribo_alloc(gInterop->Token, size, align);
}
void kuribo_module_impl_free(void* block)
{
	return gInterop->kuribo_free(gInterop->Token, block);
}
#endif


#ifdef __cplusplus
inline void* operator new (uint32_t size)
{
	return kuribo_module_impl_alloc(size, 8);
}
inline void* operator new(uint32_t size, uint32_t align)
{
	return kuribo_module_impl_alloc(size, align);
}
inline void* operator new[](uint32_t size)
{
	return operator new(size);
}
inline void* operator new[](uint32_t size, uint32_t align)
{
	return operator new(size, align);
}
inline void operator delete(void* p)
{
	kuribo_module_impl_free(p);
}
inline void operator delete[](void* p)
{
	kuribo_module_impl_free(p);
}
#endif
#endif