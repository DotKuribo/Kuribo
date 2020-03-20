#include "Project.hxx"

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
	attach();
}
ModuleInstance::~ModuleInstance()
{
	KURIBO_SCOPED_LOG("Module: Destruction");
	detach();
}
bool ModuleInstance::configured() const
{
	return mInterop.id == static_cast<u32>(InteropId::Valid);
}
bool ModuleInstance::configure()
{
	// Check if already configured
	if (configured())
		return false;

	mInterop.id = static_cast<u32>(InteropId::Valid);
	mInterop.kuribo_alloc = interopAlloc;
	mInterop.kuribo_free = interopFree;
	mInterop.Token = reinterpret_cast<kuribo_token*>(100); // todo
	return true;
}

bool ModuleInstance::attach()
{
	KURIBO_SCOPED_LOG("Module: Attaching");
	return moduleCall(KURIBO_MODULE_CALL_ATTACH, &mInterop);
}
bool ModuleInstance::detach()
{
	KURIBO_SCOPED_LOG("Module: Detatching");
	return moduleCall(KURIBO_MODULE_CALL_DETACH, &mInterop);
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
bool ModuleInstance::moduleCall(kuribo_module_call t, kuribo_module_context* arg)
{
	if (!configured())
		return false;

	KURIBO_SCOPED_LOG("Calling module prologue");
	pModule->prologue(t, arg);
	return true;
}
}