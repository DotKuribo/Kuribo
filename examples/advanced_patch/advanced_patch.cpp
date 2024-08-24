#include <kuribo_sdk.h>

extern "C" void OSReport(const char*, ...);

kx::DefineModule("Advanced Patch", "examples", "1.0");

void MyOSReport(const char*, ...);

// Define a togglable patch, enabled by default
kx::togglable_ppc_b OSReportPatch((u32)&OSReport, (void*)&MyOSReport);

void MyOSReport(const char* s, ...) {
  kx::scoped_guard<kx::togglable_ppc_b, false> g(OSReportPatch);

  // Now we can freely call OSReport again
  OSReport("Intercepted OSReport with first argument %s\n", s);
}