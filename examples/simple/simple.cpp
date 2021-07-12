#include <sdk/kuribo_sdk.h>

pp::DefineModule("Simple", "examples", "1.0");

void MyFunction() {}

// Disable THP
pp::Patch32(0x80552C28, 0x60000000);

// Insert a `b MyFunction` at 0x800ECAF8
pp::PatchB(0x800ECAF8, MyFunction);

// Insert a `bl MyFunction` at 0x800ECAFC
pp::PatchBL(0x800ECAFC, MyFunction);