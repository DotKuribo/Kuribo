#include <kuribo_sdk.h>

kx::DefineModule("Toggle", "examples", "1.0");

kx::auto_patch disable_thp(/*addr = */ 0x80552C28, /*val = */ 0x60000000,
                           /*by_default = */ false);

static bool IsThpEnabled() { return !disable_thp.is_enabled(); }
static void DisableThp() { disable_thp.enable(); }
static void EnableThp() { disable_thp.disable(); }