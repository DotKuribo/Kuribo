#pragma once

#ifdef _WIN32
// TODO -- include?
#define StackAlloc _malloca
#else
#define StackAlloc alloca
#endif

// #define StackAllocAligned(size, align) StackAlloc(size) // TODO