#include <sdk/kuribo_sdk.h>

pp::DefineModule("Simple", "examples", "1.0");

pp::auto_patch disable_thp(/*addr = */ 0x80552C28, /*val = */ 0x60000000,
                           /*by_default = */ false);

static void IsThpEnabled() { return !disable_thp.is_enabled(); }
static void DisableThp() { disable_thp.enable(); }
static void EnableThp() { disable_thp.disable(); }