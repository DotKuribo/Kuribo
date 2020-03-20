#pragma once

#include "modules/Project.hxx"

#include "Loader.hxx"

namespace kuribo {

struct KamekModule final : public IModule
{
	KamekModule(const char* path)
	{

		KURIBO_SCOPED_LOG("Loading kamek");
		int size = 0;
		int rsize = 0;
		const auto buf = io::dvd::loadFile(path, &size, &rsize);
		KURIBO_ASSERT(buf && "Failed to load file.");
		KURIBO_ASSERT(rsize && "File is empty");

		{
			KURIBO_SCOPED_LOG("Loading kamek binary");

			void* text = nullptr;

			auto succ = loadKamekBinary(KamekLoadParam{ eastl::string_view((char*)buf.get(), rsize), nullptr, &text });

			if (succ != KamekLoadResult::OK)
			{
				KURIBO_LOG("Failed to load kamek binary: ");

				switch (succ)
				{
				case KamekLoadResult::MalformedRequest:
					KURIBO_LOG("Malformed Request -- Caller supplied invalid arguments.\n");
					break;
				case KamekLoadResult::InvalidFileType:
					KURIBO_LOG("Invalid File -- This is not a Kamek binary.\n");
					break;
				case KamekLoadResult::InvalidVersion:
					KURIBO_LOG("Invalid Version -- Only V1.0 are supported.\n");
					break;
				case KamekLoadResult::BadAlloc:
					KURIBO_LOG("Out of Memory -- File is too large.\n");
					break;
				case KamekLoadResult::BadReloc:
					KURIBO_LOG("Bad relocation -- Outside of Kamek's superset of a subset of the ELF spec.\n");
					break;
				default:
					KURIBO_LOG("Unknown error.\n");
					break;
				}
			}
			KURIBO_ASSERT(succ == KamekLoadResult::OK);
			KURIBO_ASSERT(text);

			mData = eastl::unique_ptr<u8[]>((u8*)text);
		}
	}
	~KamekModule() override
	{}

	void prologue(kuribo_module_call type, kuribo_module_context* interop) override final
	{
		KURIBO_SCOPED_LOG("KAMEK Module: Prologue call");
		KURIBO_LOG("Type: %u, interop: %p\n", (u32)type, interop);
		KURIBO_LOG("PROLOGUE: %p\n", mData.get());
		// Prologue is first function
		reinterpret_cast<kuribo_module_prologue>(mData.get())(type, interop);
	}
	eastl::unique_ptr<u8[]> mData;
};

}