#include <sdk/kuribo_sdk.h>

extern "C" void OSReport(const char*, ...);

pp::DefineModule("Advanced Patch", "examples", "1.0");

void MyOSReport(const char*, ...);

// Define a togglable patch, enabled by default
pp::togglable_ppc_b OSReportPatch(OSReport, MyOSReport);

void MyOSReport(const char* s, ...) {
  pp::scoped_guard<pp::togglable_ppc_b, false> g(OSReportPatch);

  // Now we can freely call OSReport again
  OSReport("Intercepted OSReport with first argument %s\n", s);
}