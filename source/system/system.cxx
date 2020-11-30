#include "system.hxx"
#include <io/filesystem.hxx>

namespace kuribo {

void DefaultAbortHandler(const char* reason) {
  Critical g;

  while (1) {
  }
}

System* System::sInstance;

System::System() {
  KURIBO_SCOPED_LOG("Initializing System");

  io::fs::InitFilesystem();
  // loadCodeTextFile("kuribo_codes.txt");
}

bool System::loadCodeTextFile(const eastl::string_view path) {
  KURIBO_SCOPED_LOG("Loading file from disc...");

  eastl::string allocstr{path.data(), path.size()};
  KURIBO_LOG("File: %s\n", allocstr.c_str());
  auto str = kuribo::io::dvd::loadFileString(allocstr.c_str());

  if (str.empty()) {
    KURIBO_LOG("Error: Cannot load file..\n");
    return false;
  }
  // Construct a lexical parser.
  // kuribo::gecko::CodeParser parser{ eastl::string_view { str } };

  // Attach an interpreter to receive parsing actions.
  // auto delegate = mProjectManager.getGeckoDelegate();
  // parser.parse(delegate);

  return true;
}

} // namespace kuribo
