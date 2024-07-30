/*
 * @file  kuribo_module.h
 *
 * @brief Exposes the kernel modules APIs. Be warned that module reloading may
 *        not be functional on all kernel versions and is just a debug feature.
 *        You probably don't want to use these APIs in production!
 */
#pragma once

#include "kuribo_types.h"

/**
 * @brief Print all loaded modules to the console.
 */
KX_API void kxSystemPrintLoadedModules(void);

/**
 * @brief Queue a reload of all loaded modules. Because invoking this directly
 *        would involve returning to a module that may have been unloaded, this
 *        just schedules a reload.
 *
 * @note  Be warned that module reloading may not be functional on all kernel
 *        versions and is just a debug feature. You probably don't want to use
 *        these APIs in production!
 */
KX_API void kxSystemReloadAllModules(void);

/* To avoid a kernel update, I'm not actually renaming the symbol. */
#define kxSystemInstallReloadHandler kxSystemSetEventCaller

/**
 * @brief Injects the following ppc instruction at a MEM1 address:
 *        `b kuribo_reload_check_pending`
 *        If a pending reload has been scheduled above, all modules will be
 *        reloaded.
 *
 * @note  Be warned that module reloading may not be functional on all kernel
 *        versions and is just a debug feature. You probably don't want to use
 *        these APIs in production!
 */
KX_API void kxSystemInstallReloadHandler(void* ppc_instruction_to_insert_b_at);
