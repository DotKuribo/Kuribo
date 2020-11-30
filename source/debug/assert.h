#pragma once

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

enum kuribo_assert_type {
  KURIBO_ASSERT_TYPE_DEFAULT,
  KURIBO_ASSERT_TYPE_POINTER
};

int kuribo_fail_assertion(const char* file, int line, const char* exp,
                          const char* msg, enum kuribo_assert_type type);

#ifdef KURIBO_ENABLE_ASSERT
#define KURIBO_ASSERT_EXT(exp, msg, type)                                      \
  ((exp) || kuribo_fail_assertion(__FILE__, __LINE__, #exp, msg, type))
#else
#define KURIBO_ASSERT_EXT(...)
#endif

#define KURIBO_ASSERT_EX(exp, msg)                                             \
  KURIBO_ASSERT_EXT(exp, msg, KURIBO_ASSERT_TYPE_DEFAULT)
#define KURIBO_ASSERT(exp) KURIBO_ASSERT_EX(exp, nullptr)
#define KURIBO_PTR_ASSERT(ptr)                                                 \
  KURIBO_ASSERT_EXT(exp, nullptr, KURIBO_ASSERT_TYPE_POINTER)

#ifdef __cplusplus
}
#endif