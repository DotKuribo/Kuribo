#define KURIBO_ENABLE_LOG

#include <array>
#include <jit/engine/compiler.hpp>

#include <EASTL/vector.h>
#include <debug/log.h>

#include <system/memory.hxx>

#include <core/patch.hxx>


const static std::array<u32, 4> SomeCode{
    0x04000069, 0x69, 0x02000069, 0x420,
    // Insert a random code here...
};

void GeckoJIT_RunTests() {
  auto *pSimulatedHeap =
      (u8 *)kuribo::mem::Alloc(1024, kuribo::mem::GetDefaultHeap(), 8);
#ifndef _WIN32
  for (int i = 0; i < 1024; i += 1)
    kuribo::flushAddr(pSimulatedHeap + i);
#endif
  memset(pSimulatedHeap, 0, 1024);
  gecko_jit::JITEngine engine(pSimulatedHeap, 1024);

  if (gecko_jit::BeginCodeList(engine)) {
    const bool success =
        gecko_jit::CompileCodeList(engine, &SomeCode[0], SomeCode.size() * 4);

    gecko_jit::EndCodeList(engine);
  }
  KURIBO_LOG("Our megafunction begins at: %p\n", pSimulatedHeap);
#if 0
  u32 *u32Heap = (u32 *)pSimulatedHeap;
  for (int i = 0; i < 1024 / 4; ++i) {
    if (u32Heap[i] == 0)
      break;
    KURIBO_LOG("DATA: %x %p\n", i * 4, u32Heap[i]);
  }
#endif
#ifdef _WIN32
  FILE *pFile = fopen("compiled.bin", "wb");

  for (int i = 0; i < 1024; i += 4) {
    std::swap(pSimulatedHeap[i], pSimulatedHeap[i + 3]);
    std::swap(pSimulatedHeap[i + 1], pSimulatedHeap[i + 2]);
  }
  fwrite(&pSimulatedHeap[0], 1, 1024, pFile);
  fclose(pFile);
#else
  {
    KURIBO_SCOPED_LOG("Calling compiled function");
    // ((void (*)(void))pSimulatedHeap)();
  }
#endif
}