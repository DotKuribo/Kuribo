#include <sdk/kuribo_sdk.h>

pp::DefineModule("SecretString", "examples", "1.0");

// Some random method
extern "C" const char* GetSecretString() { return "Hello, world"; }

// Expose to all other modules when loaded
pp::Export(GetSecretString);
