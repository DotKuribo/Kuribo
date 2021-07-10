#include "system.hxx"
#include <EASTL/string.h>
#include <LibKuribo/filesystem.hxx>

namespace kuribo {

void DefaultAbortHandler(const char* reason) {
  Critical g;

  while (1) {
  }
}

AbortHandler onAbort = DefaultAbortHandler;

} // namespace kuribo
