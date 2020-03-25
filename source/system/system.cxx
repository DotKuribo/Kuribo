#include "system.hxx"

namespace kuribo {

void DefaultAbortHandler(const char* reason)
{
	Critical g;

	while (1) {}
}


DeferredSingleton<System> System::sInstance;

System::System() {
	KURIBO_SCOPED_LOG("Initializing System");

	mProjectManager.loadCodeHandler("kuribo/codehandler.bin");
	loadCodeTextFile("kuribo_codes.txt");
}

bool System::loadCodeTextFile(const eastl::string_view path) {
	KURIBO_SCOPED_LOG("Loading file from disc...");

	eastl::string allocstr{ path.data(), path.size() };
	KURIBO_LOG("File: %s\n", allocstr.c_str());
	auto str = kuribo::io::dvd::loadFileString(allocstr.c_str());

	if (str.empty()) {
		KURIBO_LOG("Error: Cannot load file..\n");
		return false;
	}
	// Construct a lexical parser.
	kuribo::gecko::CodeParser parser{ eastl::string_view { str } };

	// Attach an interpreter to receive parsing actions.
	auto delegate = mProjectManager.getGeckoDelegate();
	parser.parse(delegate);

	KURIBO_LOG("Parsed: %u blocks\n", static_cast<u32>(mProjectManager.getCodeManager().getNum()));

	int i = 0;
	for (const auto& block : mProjectManager.getCodeManager().blocks) {
		auto& header = block.getCodeBlock().header;
		KURIBO_LOG("Block #%i: %p\n", i++, &block.getCodeBlock());
		KURIBO_LOG("- last:   %p\n", header.last);
		KURIBO_LOG("- next:   %p\n", header.next);
		KURIBO_LOG("- active: %s\n", header.IsActive ? "true" : "false");
		KURIBO_LOG("- id:     %u\n", static_cast<u16>(header.ID));
		KURIBO_LOG("- err:    %u\n", static_cast<u16>(header.ErrorType));
		// KURIBO_LOG("- file:   %s\n", block->debug.file.c_str());
		KURIBO_LOG("- name:   %s\n", block.getCodeBlock().debug.name.c_str());
		// KURIBO_LOG("- line:   %u\n", block->debug.file_line);
		bool odd = true;
		for (const auto c : block.getData()) {
			KURIBO_LOG_FUNCTION("   %c %08x%c", odd ? '|' : ' ', c, odd ? ' ' : '\n');
			if (c == 0xFE000000) {
				KURIBO_LOG_FUNCTION("\n");
				break;
			}
			odd = !odd;
		}
	}

	return true;
}

} // namespace kuribo
