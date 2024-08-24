#include <kuribo_sdk.h>

extern "C" void OSReport(const char*, ...);

kx::DefineModule("SecretStringPrinter", "examples", "1.0");

// This is defined in an entirely different .kxe file -- perhaps compiled with a
// different compiler, written in a different language.
extern "C" const char* GetSecretString();

// Module will fail to load if GetSecretString doesn't exist in another .kxe
// file
void PrintSecretString_Automatic() {
  OSReport("Secret String: %s\n", GetSecretString());
}

void PrintSecretString_Manual() {
  // Unlike the automatic case, we can manually handle what happens if such a
  // symbol is not found. In the automatic case we provide an error about a
  // missing function and quit. Here we can fallback to another approach.
  void* addr_of_fn = kx::Import("GetSecretString");

  if (!addr_of_fn) {
    OSReport("Could not find `GetSecretString` function.\n");
    return;
  }

  OSReport("Secret String: %s\n",
           reinterpret_cast<const char* (*)()>(addr_of_fn)());
}

// Call our methods when this module loads
kx::OnLoad(PrintSecretString_Automatic);
kx::OnLoad(PrintSecretString_Manual);