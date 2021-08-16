#include "RelocationExtractor.hpp"
#include <assert.h>

namespace kx {

bool RelocationExtractor::processRelocations(const ELFIO::elfio& elf) {
  for (auto& sec : elf.sections) {
    if (sec->get_name().ends_with(".eh_frame"))
      continue;
    if (sec->get_type() != SHT_RELA)
      continue;

    const auto affected_section = sec->get_info();
    if (!processRelocationSection({elf, sec}, affected_section))
      return false;
  }

  return true;
}

bool RelocationExtractor::processRelocationSection(
    ELFIO::relocation_section_accessor section, u32 affected_section) {
  for (int i = 0; i < section.get_entries_num(); ++i) {
    ELFIO::Elf64_Addr r_offset;
    ELFIO::Elf_Word r_symbol;
    ELFIO::Elf_Word r_type;
    ELFIO::Elf_Sxword r_addend;
    section.get_entry(i, r_offset, r_symbol, r_type, r_addend);

#ifndef NDEBUG
    // std::cout << "RELOC: " << r_symbol << std::endl;
    // printf("Type: %u\n", (u32)r_type);
#endif

    if (r_type == R_PPC_NONE)
      continue;

    std::string name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind;
    unsigned char type;
    ELFIO::Elf_Half section_index;
    unsigned char other;
    if (!mSymbols.get_symbol(r_symbol, name, value, size, bind, type,
                             section_index, other)) {
      printf("Cannot find symbol...\n");
      return false;
    }

#ifndef NDEBUG
    // std::cout << name << std::endl;
#endif

    // The affected section is always CODE, as we collapse all sections into
    // one; we need to translate the ELF section index into a CODE section
    // offset.

    MapEntry affected = {.section = static_cast<u8>(affected_section),
                         .offset = static_cast<u32>(r_offset)};
    MapEntry source = {.section = static_cast<u8>(section_index),
                       .offset = static_cast<u32>(value),
                       .name = name};

    if (section_index == SHN_ABS) {
      source = RelocationExtractor::MapEntry{
          .section = Section_RawAddress_StaticOrDynamic,
          .offset = static_cast<u32>(value),
          .name = name};
    } else if (mRemapper != nullptr) {
      auto wrapped_source = mRemapper(source, &name);

      if (!wrapped_source.has_value()) {
        continue; // skip over relocations to .mwcats
      }

      source = wrapped_source.value();
      source.name = name;
    }

    if (mRemapper != nullptr) {
      auto wrapped_affected = mRemapper(affected, nullptr);

      if (!wrapped_affected.has_value()) {
        continue; // skip over relocations to .mwcats
      }
      affected = wrapped_affected.value();
    }

    assert(affected.section == 0);

    bin::Relocation bin_reloc{.r_type = static_cast<u8>(r_type),
                              .affected_section = affected.section,
                              .source_section = source.section,
                              .pad = 0,
                              .affected_offset = affected.offset,
                              .source_offset = source.offset,
                              .source_addend = static_cast<u32>(r_addend)};

    Relocation reloc = bin_reloc;
    reloc.source_symbol = source.name;
    reloc.affected_symbol = affected.name;

    mRelocations.push_back(reloc);
  }

  return true;
}

} // namespace kx