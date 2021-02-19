#include "Module.hxx"
#include "modules/kxer/Loader.hxx"

namespace kuribo {

KuriboModuleLoader::Result KuriboModuleLoader::tryLoad(const u8* buf,
                                                       const u32 size,
                                                       mem::Heap* heap,
                                                       kxer::LoadedKXE& kxe) {
  KURIBO_SCOPED_LOG("Loading Kuribo binary...");

  eastl::string_view invalid_symbol = "?";

  kxer::LoadParam param{.binary = eastl::string_view((const char*)buf, size),
                        .heap = heap,
                        .invalid_symbol = &invalid_symbol};

  auto succ = kxer::Load(param, kxe);

  if (succ == kxer::LoadResult::Success) {
    return Result{.success = true};
  }

  eastl::string mesg;
  switch (succ) {
  case kxer::LoadResult::MalformedRequest:
    mesg = "Malformed Request -- Caller supplied invalid arguments.\n";
    break;
  case kxer::LoadResult::InvalidFileType:
    mesg = "Invalid File -- This is not a Kuribo binary.\n";
    break;
  case kxer::LoadResult::InvalidVersion:
    mesg = "Invalid Version -- Only V0 are supported.\n";
    break;
  case kxer::LoadResult::BadAlloc:
    mesg = "Out of Memory -- File is too large.\n";
    break;
  case kxer::LoadResult::BadReloc:
    mesg += "The module needs a function \"";
    mesg += eastl::string(invalid_symbol);
    mesg += "\", but none exist. Perhaps a missing module?\n";
    break;
  default:
    mesg = "Unknown error.\n";
    break;
  }

  return Result{.success = false, .failure_message = eastl::move(mesg)};
}
} // namespace kuribo