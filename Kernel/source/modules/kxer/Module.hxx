#pragma once

#include "common.h"
#include "modules/Project.hxx"
#include <EASTL/unique_ptr.h>

namespace kuribo {

eastl::unique_ptr<IModule> makeKuriboModule(const u8* data, u32 size);

} // namespace kuribo