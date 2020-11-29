#pragma once

#include "debug/assert.h"
#include "debug/log.h"

#ifdef KURIBO_DEBUG
#define KURIBO_IF_DEBUG(x) (x)
#else
#define KURIBO_IF_DEBUG(...)
#endif
