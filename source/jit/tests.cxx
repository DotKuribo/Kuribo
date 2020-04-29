#define KURIBO_ENABLE_LOG

#include <array>
#include <jit/engine/compiler.hpp>

#include <EASTL/vector.h>
#include <debug/log.h>

#include <system/memory.hxx>

#include <core/patch.hxx>


const static std::array<u32, 130> SomeCode{
  0x04002F10, 0xDEADBEEF,
  0x04002F18, 0xCAFEBABE,
  0x04002E00, 0xDEADCAFE,
  0x040004c0, 0xDEADDEAD,
  0xC200960C, 0x00000006,
  0x81950000, 0x7EA3AB78,
  0x818C0018, 0x7D8903A6,
  0x4E800421, 0x81830000,
  0x818C0008, 0x7D8903A6,
  0x4E800421, 0x81950000,
  0x60000000, 0x00000000,
  0xC200962C, 0x00000006,
  0x4E800421, 0x81950000,
  0x7EA3AB78, 0x818C0018,
  0x7D8903A6, 0x4E800421,
  0x81830000, 0x818C0010,
  0x7D8903A6, 0x4E800421,
  0x60000000, 0x00000000,
  0xC200968C, 0x00000006,
  0x4E800421, 0x81950000,
  0x7EA3AB78, 0x818C0018,
  0x7D8903A6, 0x4E800421,
  0x81830000, 0x818C0014,
  0x7D8903A6, 0x4E800421,
  0x60000000, 0x00000000,
  0xC20097A4, 0x00000007,
  0x4E800421, 0x81950000,
  0x7EA3AB78, 0x818C0018,
  0x7D8903A6, 0x4E800421,
  0x39800001, 0x9983010A,
  0x81830000, 0x818C000C,
  0x7D8903A6, 0x4E800421,
  0x60000000, 0x00000000,
    0xC0000000, 0x0e, 0x3c004e80, 0x60000020,
    0x900f0000, 0x3d808000, 0x618c3000, 0x3c00017f,
    0x6000cffc, 0x7c0903a6, 0x3d607474, 0x616b7073,
    0x800c0000, 0x7c005800, 0x40a20034, 0x394c0003,
    0x392c0002, 0x7d455378, 0x38600000, 0x8c050001,
    0x2c000000, 0x38630001, 0x4082fff4, 0x8c0a0001,
    0x9c090001, 0x3463ffff, 0x4082fff4, 0x398c0001,
    0x4200ffc0, 0x4e800020, 0xC25B5534, 0x03,
    0x48000009, 0x40000000, 0x7d8802a6, 0xc04c0000,
    0x60000000, 0x00000000, 0xc6002f00, 0x817fff00,
    0x28348200, 0xFF7E0081, 0x047308C0, 0x60000000,
    0xe0000000, 0x00000000, 0x28348200, 0xf77e0881,
    0x047308C0, 0x4BFF9CB1, 0xe0000000, 0x80008000,
    0x28348200, 0xFF7D0082, 0x045944f4, 0x38000000,
    0xe0000000, 0x00000000, 0x28348200, 0xF77D0882,
    0x048944f4, 0x38000001, 0xe0000000, 0x80008000
    // Insert a random code here...
};

static u8* pSimulatedHeap;

void eachFrameCaller() {
  // Some codes change their own instructions...
  for (int i = 0; i < 1024; i += 32)
    kuribo::flushAddr(pSimulatedHeap + i);
  ((void (*)(void))pSimulatedHeap)();
}

void GeckoJIT_RunTests() {
  pSimulatedHeap =
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
#endif
  u32* viHookBlr = FindHookInMemory(gecko_jit::GeckoHookType::VI_WII);
  KURIBO_LOG("Found VI Hook begin at... %p\n", viHookBlr);
  kuribo::directBranchEx(viHookBlr, (void*)(u32)&eachFrameCaller, false);
}