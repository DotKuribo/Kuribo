#pragma once

#include "config.h"
#include "types.h"
#include "io/io.hxx"

#include "api/Module.h"
#include <EASTL/vector.h>
#include <EASTL/memory.h>
#include <cstdio>

#include "gecko/CodeManager.hxx"

namespace kuribo {

struct IModule
{
	virtual ~IModule() = default;
	virtual void prologue(kuribo_module_call type, kuribo_module_context* interop) = 0;
};
void* interopAlloc(kuribo_token* token, u32 size, u32 align);
void interopFree(kuribo_token* token, void* block);

struct ModuleInstance
{
	ModuleInstance(eastl::unique_ptr<IModule> m);
	~ModuleInstance(); // detach on destruction
	// TODO: Check this
	//	ModuleInstance(ModuleInstance&& other)
	//		: pModule(eastl::move(other.pModule)), mInterop(other.mInterop)
	//	{}

	enum class InteropId
	{
		Unitialized = 0,
		Valid = 'MDIO'
	};

	eastl::unique_ptr<IModule> pModule;
	kuribo_module_context mInterop {};

	bool configured() const;
	bool configure();
	bool attach();
	bool detach();
	bool reload();
	// When loading a new version
	bool transitionTo(eastl::unique_ptr<IModule> pOther);

private:
	bool moduleCall(kuribo_module_call t, kuribo_module_context* arg);
};

class ProjectManager
{
public:
	bool attachModule(eastl::unique_ptr<IModule> module)
	{
		// ModuleInstance ctor configs/attaches
		mModules.emplace_back(eastl::make_unique<ModuleInstance>(std::move(module)));
		return true;
	}

	gecko::CodeManagerDelegate getGeckoDelegate() {
		return { mGeckoManager, "TODO.txt" };
	}

	bool loadCodeHandler(const eastl::string_view path);
	const gecko::CodeManager& getCodeManager() const { return mGeckoManager; }
private:
	eastl::vector<eastl::unique_ptr<ModuleInstance>> mModules;
	gecko::CodeManager mGeckoManager;
};


} // namespace kuribo