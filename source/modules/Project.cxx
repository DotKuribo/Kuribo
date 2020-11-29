#include "Project.hxx"

#include "io/io.hxx"

#include "modules/SymbolManager.hxx"

namespace kuribo {

void* interopAlloc(kuribo_token* token, u32 size, u32 align)
{
	KURIBO_ASSERT(token && "Invalid token");
	if (!token)
		return nullptr;

	//	mem::Heap& hostHeap = [](NaruhodoHeapType t) -> mem::Heap& {
	//		switch (t)
	//		{
	//		case NaruhodoHeapType::KURIBO_HEAP_TYPE_CORE:
	//			return mem::GetCoreHeap();
	//		case NaruhodoHeapType::KURIBO_HEAP_TYPE_INTERNAL:
	//			return mem::GetInternalHeap();
	//		default:
	//			KURIBO_ASSERT(!"Invalid heap");
	//			return mem::GetInternalHeap();
	//		}
	//	}(heap);

	mem::Heap& hostHeap = mem::GetDefaultHeap();

	return mem::Alloc(size, hostHeap, align);
}
void interopFree(kuribo_token* token, void* block)
{
	KURIBO_ASSERT(token && "Invalid token");
	if (!token)
		return;

	KURIBO_ASSERT(block);

	mem::Free(block);
}

ModuleInstance::ModuleInstance(eastl::unique_ptr<IModule> m)
	: pModule(eastl::move(m))
{
	configure();

	__kuribo_simple_meta_v0 meta;
  mInterop.udata.fillin_meta = &meta;
  const bool res = moduleCall(KURIBO_REASON_INQUIRE_META_DESC, &mInterop);
	KURIBO_ASSERT(res && "Unable to inquire metadata");
  if (res) {
		KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");
    KURIBO_PRINTF("[KURIBO] Loading module\n");
    KURIBO_PRINTF("         Name:     \t%s\n", meta.module_name);
    KURIBO_PRINTF("         Author:   \t%s\n", meta.module_author);
    KURIBO_PRINTF("         Version:  \t%s\n", meta.module_version);
    KURIBO_PRINTF("                     \n");
    KURIBO_PRINTF("         Built:    \t%s\n", meta.build_date);
    KURIBO_PRINTF("         Compiler: \t%s\n", meta.compiler_name);
    KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");
  }
  mInterop.udata.fillin_meta = nullptr;
	attach();
}
ModuleInstance::~ModuleInstance()
{
	KURIBO_SCOPED_LOG("Module: Destruction");
	detach();
}
bool ModuleInstance::configured() const
{
  return mInterop.register_procedure != nullptr;
}
bool ModuleInstance::configure()
{
	// Check if already configured
	if (configured())
		return false;

  mInterop.core_version = KURIBO_CORE_VERSION;
  mInterop.register_procedure = kxRegisterProcedure;
  mInterop.get_procedure = kxGetProcedure;
	return true;
}

bool ModuleInstance::attach()
{
	KURIBO_SCOPED_LOG("Module: Attaching");
	return moduleCall(KURIBO_REASON_LOAD, &mInterop);
}
bool ModuleInstance::detach()
{
	KURIBO_SCOPED_LOG("Module: Detatching");
  return moduleCall(KURIBO_REASON_UNLOAD, &mInterop);
}
bool ModuleInstance::reload()
{
	KURIBO_SCOPED_LOG("Module: Reloading");
	return detach() && attach();
}
// When loading a new version
bool ModuleInstance::transitionTo(eastl::unique_ptr<IModule> pOther)
{
	if (!configured())
		return false;

	if (!detach())
		return false;
	KURIBO_ASSERT(pModule.get());
	pModule.reset();

	pModule = std::move(pOther);
	return attach();
}
bool ModuleInstance::moduleCall(__KReason t, __kuribo_module_ctx_t* arg)
{
	if (!configured())
		return false;

	KURIBO_SCOPED_LOG("Calling module prologue");
	pModule->prologue(t, arg);
	return true;
}
}