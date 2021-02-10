#include <EASTL/unique_ptr.h>
#include <core/patch.hxx>
#include <modules/SymbolManager.hxx>
#include <modules/kxer/Loader.hxx>

namespace kuribo::kxer {

static inline mem::unique_ptr<u8>
decompressSection(const LoadParam& param, u32 file_size,
                  const kx::bin::Header& header,
                  const kx::bin::Section& section, u32* sizeCb = nullptr) {
  if (section.file_offset + section.file_size > file_size) {
    KURIBO_LOG("Size exceeds size of the file\n");
    return nullptr;
  }

  auto* _buf = new (*param.heap, section.alignment) u8[section.file_size];
  auto pBuf = mem::unique_ptr<u8>(_buf, param.heap);

  if (!pBuf) {
    KURIBO_LOG("Failed to allocate memory\n");
    return nullptr;
  }

  if (section.flags & static_cast<u32>(kx::bin::SectionFlags::CompressedSZS)) {
    KURIBO_LOG("Compressed sections are not yet supported\n");
    return nullptr;
  } else {
    memcpy(pBuf.get(),
           reinterpret_cast<const u8*>(&header) + section.file_offset,
           section.file_size);
  }

  if (sizeCb != nullptr) {
    *sizeCb = section.file_size;
  }

  return pBuf;
}

kuribo::kxer::LoadResult
handleRelocation(const kx::bin::Relocation* reloc, u8* pCode, u8* pImports,
                 const kx::bin::Header* pHeader,
                 const kuribo::kxer::LoadParam& param) {
  KURIBO_ASSERT(pCode);
  union addr {
    u32 d32;
    u32* p32;
    u16* p16;
    u8* p8;
  };

  addr affected;

  if (reloc->affected_section == 0) {
    affected.p8 = pCode + reloc->affected_offset;
  } else {
    KURIBO_LOG("Affected section must be 0\n");
    KURIBO_LOG("Reloc @ %p\n", reloc);
    KURIBO_LOG("File offset: %i\n", ((u8*)reloc) - ((u8*)pHeader));
    return LoadResult::BadReloc;
  }

  u32 source;
  if (reloc->source_section == 0) {
    source = reinterpret_cast<u32>(pCode) + reloc->source_offset +
             reloc->source_addend;
  } else if (reloc->source_section == 0xFF) {
    if (pImports == nullptr) {
      KURIBO_LOG("Imports section doesn't exist, but is referenced\n");
      return LoadResult::BadReloc;
    }
    auto* tbl = reinterpret_cast<kx::bin::BinaryString*>(pImports);
    KURIBO_LOG("TBL %p\n", tbl);
    KURIBO_LOG("STRING ID: %u\n", reloc->source_offset);
    const auto symbol = kx::bin::resolveBinaryString(
        tbl[reloc->source_offset], pHeader, param.binary.size());

    KURIBO_LOG("Symbol: %s (%u)\n", symbol.begin(), symbol.size());
    auto resolved = SymbolManager::getStaticInstance().getProcedure(
        tbl[reloc->source_offset].crc32);
    if (resolved == 0) {
      KURIBO_LOG("Failed to link: unknown symbol!\n");
      return LoadResult::BadReloc;
    }
    source = resolved + reloc->source_addend;
  }

  KURIBO_LOG("Reloc: %u, Source: %p, Dest: %p\n", reloc->r_type, (void*)source,
             affected.p8);
  switch (reloc->r_type) {
  case R_PPC_NONE:
    break;
  case R_PPC_ADDR32: {
    *affected.p32 = source;
    break;
  }
  case R_PPC_ADDR24: {
    *affected.p32 = (*affected.p32 & ~0x03fffffc) | (source & 0x03fffffc);
    break;
  }
  case R_PPC_ADDR16: // Identical..
  case R_PPC_ADDR16_LO: {
    *affected.p16 = static_cast<u16>(source);
    break;
  }
  case R_PPC_ADDR16_HI: {
    *affected.p16 = static_cast<u16>(source >> 16);
    break;
  }
  case R_PPC_ADDR16_HA: {
    *affected.p16 = (source >> 16) + !!(source & 0x8000);
    break;
  }
  case R_PPC_ADDR14:
  case R_PPC_ADDR14_BRTAKEN:
  case R_PPC_ADDR14_BRNKTAKEN: {
    *affected.p32 = (*affected.p32 & ~0x0000'fffc) | (source & 0x0000'fffc);
    break;
  }
  case R_PPC_REL24: {
    const u32 low = source - affected.d32;
    KURIBO_LOG("LOW: %u\n", low);
    *affected.p32 = (*affected.p32 & ~0x03ff'fffc) | (low & 0x03ff'fffc);
    break;
  }
  case R_PPC_REL14: {
    const u32 low = source - affected.d32;
    *affected.p32 = (*affected.p32 & ~0x0000'fffc) | (low & 0x0000'fffc);
    break;
  }
  case R_PPC_REL32: {
    KURIBO_LOG("REL32\n");
    *affected.p32 = source - affected.d32;
    break;
  }
  default:
    KURIBO_LOG("Invalid relocation: %u\n", reloc->r_type);
    return LoadResult::BadReloc;
  }
#if KURIBO_PLATFORM_WII
  dcbst(affected.p32);
  asm("sync");
  icbi(affected.p32);
#endif

  return LoadResult::Success;
}

kuribo::kxer::LoadResult
handleRelocations(const kx::bin::Relocation* pRelocs, u32 reloc_size, u8* pCode,
                  u8* pImports, const kx::bin::Header* pHeader,
                  const kuribo::kxer::LoadParam& param) {
  for (const auto* reloc = pRelocs;
       reinterpret_cast<const u8*>(reloc) <
       reinterpret_cast<const u8*>(pRelocs) + reloc_size;
       ++reloc) {
    KURIBO_LOG("Handling reloc..\n");
    const auto result =
        handleRelocation(reloc, pCode, pImports, pHeader, param);
    if (result != LoadResult::Success)
      return result;
  }

#if KURIBO_PLATFORM_WII
  asm("sync");
  asm("isync");
#endif

  return LoadResult::Success;
}

LoadResult Load(const LoadParam& param, LoadedKXE& out) {
  if (param.binary.data() == nullptr ||
      param.binary.size() < kx::bin::HEADER_SIZE)
    return LoadResult::MalformedRequest;

  if (param.heap == nullptr) {
    KURIBO_LOG("Heap was not specified\n");
    return LoadResult::MalformedRequest;
  }

  const auto* pHeader =
      reinterpret_cast<const kx::bin::Header*>(param.binary.data());

  if (pHeader->magic != 'KXER')
    return LoadResult::InvalidFileType;

  if (pHeader->kmxe_version != 0 ||
      pHeader->kernel_version > KURIBO_CORE_VERSION)
    return LoadResult::InvalidVersion;

  // Unverified: file_size
  // Unused: flags

  mem::unique_ptr<u8> pCode =
      decompressSection(param, param.binary.size(), *pHeader, pHeader->code);
  if (!pCode) {
    KURIBO_LOG("Code section does not exist\n");
    return LoadResult::InvalidFile;
  }

  u32 reloc_size = 0;
  mem::unique_ptr<u8> pRelocs = decompressSection(
      param, param.binary.size(), *pHeader, pHeader->relocations, &reloc_size);
  if (!pRelocs) {
    KURIBO_LOG("Relocation section does not exist\n");
    return LoadResult::InvalidFile;
  }

  mem::unique_ptr<u8> pImports = nullptr;

  if (pHeader->imports.file_size != 0) {
    auto _imports = decompressSection(param, param.binary.size(), *pHeader,
                                      pHeader->imports);
    if (!_imports) {
      return LoadResult::InvalidFile;
    }

    pImports = std::move(_imports);
  }

  auto result = handleRelocations(
      reinterpret_cast<const kx::bin::Relocation*>(pRelocs.get()), reloc_size,
      pCode.get(), pImports.get(), pHeader, param);

  if (result != LoadResult::Success)
    return result;

  out.prologue = reinterpret_cast<kuribo_module_prologue>(
      pCode.get() + pHeader->entry_point_offset);
  out.data = std::move(pCode);

  return LoadResult::Success;
}

} // namespace kuribo::kxer
