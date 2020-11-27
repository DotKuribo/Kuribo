#include "compiler.hpp"
#include "engine.hpp"
#include <core/patch.hxx>

#include <EASTL/array.h>
#include <EASTL/deque.h>
#include <bitset>

namespace gecko_jit {

union D_Form {
  struct {
#ifdef _WIN32
    u32 SIMM : 16;
    u32 Source : 5;
    u32 Destination : 5;
    u32 InstructionID : 6;
#else
    u32 InstructionID : 6;
    u32 Destination : 5;
    u32 Source : 5;
    u32 SIMM : 16;
#endif
  };
  u32 hex;
};
struct AND {
#ifdef _WIN32
  u32 Compare0 : 1;
  u32 Reserved : 10; // must be 28
  u32 OpB : 5;
  u32 Dest : 5;
  u32 OpA : 5;
  u32 InstructionID : 6;
#else
  u32 InstructionID : 6;
  u32 OpA : 5;
  u32 Dest : 5;
  u32 OpB : 5;
  u32 Reserved : 10; // must be 28
  u32 Compare0 : 1;
#endif
};

struct Cache {
#ifdef _WIN32
  u32 Reserved3 : 1;
  u32 AltID : 10;
  u32 RegB : 5;
  u32 RegA : 5;
  u32 Reserved1 : 5;
  u32 InstructionID : 6;
#else
  u32 InstructionID : 6;
  u32 Reserved1 : 5;
  u32 RegA : 5;
  u32 RegB : 5;
  u32 AltID : 10;
  u32 Reserved3 : 1;
#endif
};

struct CMPW {
#ifdef _WIN32
  u32 Reserved2 : 1;
  u32 AltID : 10;
  u32 RegB : 5;
  u32 RegA : 5;
  u32 ReservedL : 1;
  u32 Reserved1 : 1;
  u32 ConditionField : 3;
  u32 InstructionID : 6;
#else
  u32 InstructionID : 6;
  u32 ConditionField : 3;
  u32 Reserved1 : 1;
  u32 ReservedL : 1;
  u32 RegA : 5;
  u32 RegB : 5;
  u32 AltID : 10;
  u32 Reserved2 : 1;
#endif
};

struct CMPWI {
#ifdef _WIN32
  u32 SIMM : 16;
  u32 RegA : 5;
  u32 ReservedL : 1;
  u32 Reserved1 : 1;
  u32 ConditionField : 3;
  u32 InstructionID : 6;
#else
  u32 InstructionID : 6;
  u32 ConditionField : 3;
  u32 Reserved1 : 1;
  u32 ReservedL : 1;
  u32 RegA : 5;
  u32 SIMM : 16;
#endif
};

static_assert(sizeof(AND) == sizeof(u32),
              "Invalidly sized instruction structure");

enum {
  PPC_OP_ADDI = 14,
  PPC_OP_ADDIS = 15,
  PPC_OP_AND = 31,
  PPC_OP_ANDI = 28,
  PPC_OP_BC = 16,
  PPC_OP_B = 18,
  PPC_OP_B_MISC = 19,
  PPC_OP_CMPW = 31,
  PPC_OP_CMPWI = 11,
  PPC_OP_DCBF = 31,
  PPC_OP_DCBST = 31,
  PPC_OP_ICBI = 31,
  PPC_OP_ISYNC = 19,
  PPC_OP_LBZ = 34,
  PPC_OP_LHZ = 40,
  PPC_OP_LWZ = 32,
  PPC_OP_LFS = 48,
  PPC_OP_ORI = 24,

  PPC_OP_STB = 38,
  PPC_OP_STBU = 39,

  PPC_OP_STH = 44,
  PPC_OP_STHU = 45,

  PPC_OP_STW = 36,
  PPC_OP_STWU = 37,

  PPC_OP_STFDU = 55,

  PPC_OP_SYNC = 31
};

struct LinearExecutionState {
  // Let's keep track of the last value of registers
  // Let r14, r15, r16 they could be whatever

  bool isKnown(u8 reg) {
    return reg >= 14 && reg <= 31 && register_values_known[reg - 14] != 0;
  }
  void remember(u8 reg, u32 value) {
    if (reg < 14 || reg > 31)
      return;
    register_values_known[reg - 14] = 1;
    register_values[reg - 14] = value;
  }
  u32 recall(u8 reg) {
    if (reg < 14 || reg > 31)
      return 0;
    return register_values[reg - 14];
  }
  void forget(u8 reg) {
    if (reg >= 14 && reg <= 31)
      register_values_known[reg - 14] = 0;
  }
  void flush() { register_values_known = {}; }

  u8 *allocInstruction(u32 size) {
    return mEngine.alloc(JITEngine::CodeHeap::ExecuteEveryFrame, size);
  }
  template <typename T> T *allocInstruction() {
    return reinterpret_cast<T *>(allocInstruction(sizeof(T)));
  }
  u8 *allocData(u32 size) {
    return mEngine.alloc(JITEngine::CodeHeap::ExecuteOnDemand, size);
  }

  LinearExecutionState(JITEngine &engine) : mEngine(engine) {}

private:
  // Optimizing away loads is difficult when you can't know the next
  // instruction. You could be overwriting a constant just for the next to use
  // it! This system is primitive -- it'll break with jumps. It will just
  // prevent the most basic repetitive instructions
  eastl::array<u32, 18> register_values{}; // r14-r31
  std::bitset<32> register_values_known{};

  JITEngine &mEngine;
};

//! @brief Compile an immediate (32-bit, integral) load to a register.
//!
//! @param[in] state  State of the compiled function.
//! @param[in] reg    Register to output to
//! @param[in] value  Value to set.
//!
//! @details
//!
//! If the value is already set, do nothing.
//! If value can be stored in a SIMM without worry of sign-extension:
//! - li reg, value
//! Otherwise:
//! - lis reg, value@h
//! - ori reg, reg, value@l
//!
void compileImmediateLoad(LinearExecutionState &state, u8 reg, u32 value) {
  if (state.isKnown(reg)) {
    const u32 prior_value = state.recall(reg);
    //KURIBO_LOG("VALUE %x - PRIOR %x = %x\n", value, prior_value, value - prior_value);
    if (prior_value == value) {
      // The value is already set. We don't need to do anything.
      return;
    }
    else if ((static_cast<s32>(value - prior_value) < 0x7fff) && (static_cast<s32>(value - prior_value) > -0x7fff)) {
      
      state.remember(reg, value);

      D_Form* addi = state.allocInstruction<D_Form>();
      addi->InstructionID = PPC_OP_ADDI;
      addi->Destination = reg;
      addi->Source = reg;
      addi->SIMM = (value - prior_value) & 0xffff;

      return;
    }
  }
  state.remember(reg, value);

  if (value < 0x7FFF) {
    D_Form *li = state.allocInstruction<D_Form>();
    li->InstructionID = PPC_OP_ADDI;
    li->Destination = reg;
    li->Source = 0;
    li->SIMM = value & 0xffff;
  } else {
    D_Form *lis = state.allocInstruction<D_Form>();
    lis->InstructionID = PPC_OP_ADDIS;
    lis->Destination = reg;
    lis->Source = 0;
    lis->SIMM = (value >> 16) & 0xffff;

    if (value & 0xffff) {
      D_Form *ori = state.allocInstruction<D_Form>();
      ori->InstructionID = PPC_OP_ORI;
      ori->Destination = reg;
      ori->Source = reg;
      ori->SIMM = value & 0xffff;
    }
  }
}

void compileMemoryLoad(LinearExecutionState &state, u8 reg, u32 addr, u8 size) {
  if ((addr & 0xffff) > 0x7FFF) {
    addr += 0x10000;
  }
  compileImmediateLoad(state, reg, (addr & 0xffff0000));
  D_Form *lwz = state.allocInstruction<D_Form>();
  switch (size) {
  case 1:
    lwz->InstructionID = PPC_OP_LBZ;
    break;
  case 2:
    lwz->InstructionID = PPC_OP_LHZ;
    break;
  case 4:
  default:
    lwz->InstructionID = PPC_OP_LWZ;
    break;
  }
  lwz->Destination = reg;
  state.forget(reg);
  lwz->Source = reg;
  lwz->SIMM = (addr & 0xffff);
}

void compileCompare(LinearExecutionState& state, u8 regA, u8 regB, u8 crf, u32 value) {
  if (value <= 0x7fff) {
    CMPWI* cmpwi = state.allocInstruction<CMPWI>(); // cmpwi r14, value

    cmpwi->InstructionID = PPC_OP_CMPWI;
    cmpwi->ConditionField = crf;
    cmpwi->Reserved1 = 0;
    cmpwi->ReservedL = 0;
    cmpwi->RegA = regA;
    cmpwi->SIMM = value;

  } else {
    compileImmediateLoad(state, 15, value);

    CMPW* cmpw = state.allocInstruction<CMPW>(); // cmplw r14, r15

    cmpw->InstructionID = PPC_OP_CMPW;
    cmpw->ConditionField = crf;
    cmpw->Reserved1 = 0;
    cmpw->ReservedL = 0;
    cmpw->RegA = regA;
    cmpw->RegB = regB;
    cmpw->Reserved2 = 0;
  }
}

void compileMask(LinearExecutionState &state, u8 reg, u8 regMask, u32 mask) {
  // TODO -- use rlwinm when possible

  if (mask <= 0x7FFF) {
    D_Form* andi = state.allocInstruction<D_Form>();
    andi->InstructionID = PPC_OP_ANDI;
    andi->Destination = reg;
    andi->Source = reg;
    andi->SIMM = mask & 0xffff;
    return;
  }
  compileImmediateLoad(state, regMask, mask);
  AND *_and = state.allocInstruction<AND>(); // and reg, reg, regMask
  _and->InstructionID = PPC_OP_AND;
  _and->OpA = reg;
  _and->OpB = regMask;
  _and->Dest = reg;
  state.forget(reg);
  _and->Reserved = 28;
  _and->Compare0 = 0;
}

//! @brief Generates load/store instruction packages
//!
//! @param[in] engine    The JIT engine
//! @param[in] addrReg   Register to set address with
//! @param[in] valReg    The register to load/store with
//! @param[in] address   Address to load/store from
//! @param[in] value     Value to store at address
//! @param[in] size      Size of the value to store,
//!                      should be 1, 2, 4
//!
//! @details
//!
//! Generates a store of the given value
//! to the given address using the
//! designated registers
//!
void compileStore(LinearExecutionState &engine, u8 addrReg, u8 valReg,
                  u32 address, u32 value, u8 size) {

  compileImmediateLoad(engine, valReg, value); // this creates our value
  
  bool need_address_load = true;

  // writeback optimization
  if (engine.isKnown(addrReg)) {
    const s32 delta = address - engine.recall(addrReg);
    if (const s16 truncated = static_cast<s16>(delta); truncated == delta) {
      D_Form *stwu = engine.allocInstruction<D_Form>();
      if (size == sizeof(u8)) {
        stwu->InstructionID = PPC_OP_STBU;
      } else if (size == sizeof(u16)) {
        stwu->InstructionID = PPC_OP_STHU;
      } else {
        stwu->InstructionID = PPC_OP_STWU;
      }
      stwu->Destination = valReg;
      stwu->Source = addrReg;
      stwu->SIMM = truncated;
      engine.remember(addrReg, address);
      need_address_load = false;
    }
  }

  if (need_address_load) {
    compileImmediateLoad(engine, addrReg, address);
    D_Form* stw = engine.allocInstruction<D_Form>();

    if (size == sizeof(u8)) {
      stw->InstructionID = PPC_OP_STB;
    }
    else if (size == sizeof(u16)) {
      stw->InstructionID = PPC_OP_STH;
    }
    else {
      stw->InstructionID = PPC_OP_STW;
    }
    stw->Destination = valReg;
    stw->Source = addrReg;
    stw->SIMM = 0;
  }
#ifdef CACHE_SAFE
  Cache* icbi = engine.allocInstruction<Cache>();

  icbi->InstructionID = PPC_OP_ICBI;
  icbi->RegA = 0;
  icbi->RegB = addrReg;
  icbi->Reserved1 = 0;
  icbi->AltID = 982;
  icbi->Reserved3 = 0;
#endif
}

void compileBranch(u32 *address, const char *target, bool link = true) {
  kuribo::directBranchEx(address, (void *)target, link);
}

void compileBranch(LinearExecutionState &state, u32 target, bool link = true) {
  u32 *bl = state.allocInstruction<u32>();
  const u32 offset = (u32)target - (u32)bl;
  *bl = ((offset & 0x3ffffff) | 0x48000000 | link);
}

//! @brief Generates an array write
//!
//! @param[in] engine     The JIT engine
//! @param[in] sourceReg  Register to set address with
//! @param[in] destReg    The register to load/store with
//! @param[in] address    Address to load/store from
//! @param[in] valueArray Value to store at address
//! @param[in] valueCount Number of values in the array.
//!
void compileArrayCopy(LinearExecutionState &engine, u8 sourceReg, u8 destReg,
                      u32 address, u8 *valueArray, size_t value_count) {
  // Use a memcpy if too big
  if (value_count > 32 * 4) {
    u8 *data = engine.allocData(value_count);
    memcpy(data, valueArray, value_count);
    // Now we know that data won't be freed on us or overwritten.
    // (we have no such guarantee for valueArray)
    // We need to actually generate the bl
    compileImmediateLoad(engine, 3, address);     // dst
    compileImmediateLoad(engine, 4, (u32)data);   // src
    compileImmediateLoad(engine, 5, value_count); // size
    compileBranch(engine, (u32)&memcpy, true);
    return;
  }

  for (u32 i = 0; i < value_count / 4; ++i) {
    const u32 value = reinterpret_cast<u32*>(valueArray)[i];
    compileStore(engine, sourceReg, destReg, address + i * 4, value,
                 sizeof(u32));
  }
  if (value_count % 4 >= 2) {
    const u32 offset = value_count - value_count % 4;
    const u16 value = reinterpret_cast<u16*>(valueArray)[offset / 2];
    compileStore(engine, sourceReg, destReg, address + offset * 2, value,
                 sizeof(u16));
  }
  if (value_count % 1 != 0) {
    const u32 offset = value_count - 1;
    compileStore(engine, sourceReg, destReg, address + offset, valueArray[offset],
                 sizeof(u8));
  }
}

#ifndef _WIN32
__attribute__((noinline))
#endif
// void __memset(u8* dst, u8 fill, u32 count) {
//   std::fill(dst, dst + count, fill);
// }
#define __memset memset
#ifndef _WIN32
__attribute__((noinline))
#endif
void __memset2(u16* dst, u16 fill, u32 count) {
  std::fill(dst, dst + count, fill);
}
#ifndef _WIN32
__attribute__((noinline))
#endif
void __memset4(u32* dst, u32 fill, u32 count) {
  std::fill(dst, dst + count, fill);
}

void compileArraySet(LinearExecutionState &engine, u32 address, u32 value,
                     size_t value_count, u8 value_size) {

  // Create memset call
  compileImmediateLoad(engine, 3, address);     // dst
  compileImmediateLoad(engine, 4, value);       // fill
  compileImmediateLoad(engine, 5, value_count); // size

  switch (value_size) {
  case 1:
    compileBranch(engine, (u32)&__memset, true);
    break;
  case 2:
    compileBranch(engine, (u32)&__memset2, true);
    break;
  case 4:
  default:
    compileBranch(engine, (u32)&__memset4, true);
    break;
  }
}

void compileSerialWrite(LinearExecutionState &engine, u8 sourceReg, u8 destReg,
                        u32 address, u32 value, u16 value_count,
                        u16 address_increment, u32 value_increment,
                        u8 value_size) {
  for (u32 i = 0; i < value_count; ++i) {
    compileStore(engine, sourceReg, destReg, address + (i * address_increment),
                 value + (i * value_increment), value_size);
  }
}

void compileExecuteASM(LinearExecutionState &engine, u8 *funcPointer,
                       u32 size) {
  u8 *data = engine.allocData(size * 8);
  memcpy(data, (u8 *)funcPointer, (size * 8));
  // KURIBO_LOG("data = 0x%X, funcPointer = 0x%X, size = 0x%X funcPointer data =
  // %x", data,funcPointer,(size*8));
  compileImmediateLoad(engine, 15,
                       (u32)data); // r15 = start of code, for compatibility
  compileBranch(engine, (u32)data, true);
}

void compileInjectASM(LinearExecutionState &engine, u8 *funcPointer,
                      u32 *address, u32 size) {
  u8 *data = engine.allocData(size * 8);
  memcpy(data, (u8 *)funcPointer, (size * 8));
  compileBranch((u32*)((u32)address & ~1), (const char *)data, ((u32)address & 1) == 1);
  compileBranch(
      (u32 *)(data + (size * 8) - 4), (const char *)(address + 1),
      false); // calculates offsets to branch from end of code to address + 4
}

// dcbf, sync, icbi, sync, isync
void compileCacheClear(LinearExecutionState &state, u8 reg) {
  *state.allocInstruction<u32>() = 0x3333; // Put an assembled instruction here
}

//! stwu r1, -0x20 (r1)
//! mflr r0
//! stw r0, 0x8 (r1)
//! stw r14, 0xC (r1)
//! stw r15, 0x10 (r1)
static u32 __prologue[5]{0x9421FFE0, 0x7C0802A6, 0x90010008,
                         0x91C1000C, 0x91E10010};
//! lwz r15, 0x10 (r1)
//! lwz r14, 0xC (r1)
//! lwz r0, 0x8 (r1)
//! mtlr r0
//! addi r1, r1, 0x20
//! blr
static u32 __epilogue[6]{0x81E10010, 0x81C1000C, 0x80010008,
                         0x7C0803A6, 0x38210020, 0x4E800020};

bool BeginCodeList(JITEngine &engine) {
  u32 *block =
      (u32 *)engine.alloc(JITEngine::CodeHeap::ExecuteEveryFrame, sizeof(__prologue));
  memcpy(block, &__prologue[0], sizeof(__prologue));
  return true;
}
bool EndCodeList(JITEngine &engine) {
  u32 *block =
      (u32 *)engine.alloc(JITEngine::CodeHeap::ExecuteEveryFrame, sizeof(__epilogue));
  memcpy(block, &__epilogue[0], sizeof(__epilogue));
  return true;
}

#ifndef _WIN32
__attribute__((noinline))
#endif
bool CompileCodeList(JITEngine& engine, const u32* list, size_t size) {
  LinearExecutionState state(engine);

  u32 bp = 0x80000000;

  auto Handle0002 = [&](u32 op, u32 address, u32 valueInfo) {
    u32 thing_size = 1;
    u32 mask = 0xff;
    if (op == 0x02) {
      thing_size = 2;
      mask = 0xffff;
    }
    compileArraySet(state, address, (valueInfo & mask),
                    ((valueInfo >> 16) & 0xFFFF), thing_size);
  };
  auto Handle04 = [&](u32 address, u32 value) {
    compileStore(state, 14, 15, address, value, sizeof(u32));
  };

  auto Handle06 = [&](u32 address, u32 value_count, u8 *value_array) {
    compileArrayCopy(state, 14, 15, address, value_array, value_count);
  };

  auto Handle08 = [&](u32 address, u32 value, u32 writeInfo,
                      u32 value_increment) {
    compileSerialWrite(state, 14, 15, address, value,
                       ((writeInfo >> 16) & 0xFFF), (writeInfo & 0xFFFF),
                       value_increment, ((writeInfo >> 28) & 3));
  };

  eastl::deque<u32 *> openIfStatements;

  // if (*address == value)
  auto Handle20 = [&](u32 *address, u32 value) {
    // KURIBO_LOG("<IF> address = %x, value = %x\n", (u32)address, (u32)value);

    compileMemoryLoad(state, 14, (u32)address, sizeof(u32));
    compileCompare(state, 14, 15, 0, value);

    openIfStatements.push_back(state.allocInstruction<u32>());
  };

  // if ((*address & mask) == value)
  auto Handle28 = [&](u16 *address, u16 mask, u16 value) {
    //KURIBO_LOG("<IF> address = %x, mask = %x, value = %x\n", (u32)address,
    //           (u32)mask, (u32)value);

    compileMemoryLoad(state, 14, (u32)address, sizeof(u16));
    compileMask(state, 14, 15, (u16)~mask); // stencil
    compileCompare(state, 14, 15, 0, value);

    openIfStatements.push_back(state.allocInstruction<u32>());
  };

  auto HandleC0 = [&](u32 funcpointer, u32 size) {
    compileExecuteASM(state, (u8 *)funcpointer, size);
  };

  auto HandleC2 = [&](u32 address, u32 funcpointer, u32 size) {
    compileInjectASM(state, (u8 *)funcpointer, (u32 *)address, size);
  };

  auto HandleC6 = [&](u32 address, u32 target) {
    compileBranch((u32 *)(address & ~1), (const char *)target, address & 1);
  };

  auto HandleE0 = [&](u16 bp, u16 po) {
    // KURIBO_LOG("<ENDIF> Base Address = %x0000, Pointer Address = %x0000\n",
    //            bp, po);

    u32 *paired = openIfStatements.back();
    openIfStatements.pop_back();

    // compile a bne to here
    u32 *addr = state.allocInstruction<u32>(); // Hack generate a nop here to
                                               // get the address..
    *addr = 0x60000000;

    *paired = 0x40820000 | ((((char *)addr - (char *)paired) & ~3) & 0xffff);

    // we can't assume register values
    state.flush();
  };
  u32 i = 0;
  while (i < size / 4) {
    // round up to line
    if (i % 2) ++i;
    if (i >= size / 4) break;

    const u32 op = (list[i] & 0xfe000000) >> 24;
    switch (op) {
    case 0x00:
    case 0x02:
      Handle0002(op, bp + (list[i] & 0x01ffffff), list[i + 1]);
      i += 2;
      break;
    case 0x04:
      Handle04(bp + (list[i] & 0x01ffffff), list[i + 1]);
      i += 2;
      break;
    case 0x06: {
      Handle06(bp + (list[i] & 0x01ffffff), list[i + 1], (u8*)&list[i + 2]);
      const auto bytes = list[i + 1];
      i += 2 + ((bytes + 7) & ~7) / 4;
      break;
    }
    case 0x08:
      Handle08(bp + (list[i] & 0x01ffffff), list[i + 1], list[i + 2],
               list[i + 3]);
      i += 4;
      break;
    case 0x20:
      Handle20((u32 *)(bp + (list[i] & 0x01ffffff)), list[i + 1]);
      i += 2;
      break;
    case 0x28:
      Handle28((u16 *)(bp + (list[i] & 0x01ffffff)),
               (u16)((list[i + 1] >> 16) & 0xffff),
               (u16)(list[i + 1] & 0xffff));
      i += 2;
      break;
    case 0xC0:
      HandleC0((u32)&list[i + 2], list[i + 1]);
      i += 2 + (list[i + 1] * 2);
      break;
    case 0xC2:
      HandleC2(bp + (list[i] & 0x01ffffff), (u32)&list[i + 2], list[i + 1]);
      i += 2 + (list[i + 1] * 2);
      break;
    case 0xC4:
      HandleC2((bp + (list[i] & 0x01ffffff)) | 1, (u32)&list[i + 2], list[i + 1]);
      i += 2 + (list[i + 1] * 2);
      break;
    case 0xC6:
      HandleC6(bp + (list[i] & 0x01ffffff), list[i + 1]);
      i += 2;
      break;
    case 0xC8:
      HandleC6((bp + (list[i] & 0x01ffffff)) | 1, list[i + 1]);
      i += 2;
      break;
    case 0xE0:
      HandleE0((u16)((list[i + 1] >> 16) & 0xffff),
               (u16)(list[i + 1] & 0xffff));
      i += 2;
      break;
    default:
      // Unknown code, exit early
      return false;
    }
  }

  return true;
}

} // namespace gecko_jit
