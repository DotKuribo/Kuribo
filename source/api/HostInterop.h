#pragma once

#include <stdint.h>


// LockMutex/UnlockMutex not used for now
// Or Reschedule

#define FFI_NAME(name) kuribo_ffi_##name

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#define EXTERN_SYM_DECL(name) EXTERN_C const uint32_t FFI_NAME(name);

EXTERN_SYM_DECL(os_report)
EXTERN_SYM_DECL(os_fatal)
EXTERN_SYM_DECL(dvd_path_to_entrynum)
EXTERN_SYM_DECL(dvd_fast_open)
EXTERN_SYM_DECL(dvd_read_prio)
EXTERN_SYM_DECL(dvd_close)

#define EXTERN_SYM_DEF(name, value) EXTERN_C const uint32_t FFI_NAME(name) = value;

// Simple API
#define KURIBO_SET_OS(report, fatal) \
	EXTERN_SYM_DEF(os_report, report) \
	EXTERN_SYM_DEF(os_fatal, fatal)

#define KURIBO_SET_DVD(path_to_entrynum, fast_open, read_prio, close) \
	EXTERN_SYM_DEF(dvd_path_to_entrynum, path_to_entrynum) \
	EXTERN_SYM_DEF(dvd_fast_open, fast_open) \
	EXTERN_SYM_DEF(dvd_read_prio, read_prio) \
	EXTERN_SYM_DEF(dvd_close, close)
