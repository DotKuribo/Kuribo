#include <kuribo_sdk.h>

kx::DefineModule("Simple", "examples", "1.0");

void MyFunction() {}

// Disable THP
kx::Patch32(0x80552C28, kx::SkipInstruction);

// Insert a `b MyFunction` at 0x800ECAF8
kx::PatchB(0x800ECAF8, MyFunction);

// Insert a `bl MyFunction` at 0x800ECAFC
kx::PatchBL(0x800ECAFC, MyFunction);