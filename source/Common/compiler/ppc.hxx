#pragma once

#include "common.h"

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
