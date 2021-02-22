#include "Converter.hpp"
#include "BinaryBuilder.hpp"
#include "RelocationExtractor.hpp"
#include "crc.hpp"
#include <assert.h>
#include <sstream>

namespace kx {

void Converter::readSymbols(const std::string& path) {
  std::ifstream stream(path);

  std::string line;
  while (std::getline(stream, line)) {
    std::string symbol = line.substr(0, line.find("="));
    std::string value = line.substr(symbol.size());
    if (value.size() > 3)
      mExplicitSymbols[symbol] = strtoul(value.c_str() + 3, nullptr, 16);
  }
}

bool Converter::addElf(const char* path) {
  if (!mElf.load(path))
    return false;

  if (mElf.get_class() != ELFCLASS32)
    return false;

#ifndef NDEBUG
  printElfInfo(mElf);
#endif

  return true;
}

void Converter::printElfInfo(const ELFIO::elfio& elf) const {
  printf("Number of sections: %u\n",
         static_cast<unsigned>(elf.sections.size()));

  for (const auto& sec : elf.sections) {
    printf("%s (%x): %u bytes\n", sec->get_name().c_str(),
           static_cast<unsigned>(sec->get_type()),
           static_cast<unsigned>(sec->get_size()));

    if (sec->get_name() == ".eh_frame") {
      printf("ERROR: Compile with -fno-exceptions\n");
    }
  }

  printf("Number of segments: %u\n",
         static_cast<unsigned>(elf.segments.size()));

  for (const auto& sec : elf.segments)
    printf("(%x): %u bytes\n", static_cast<unsigned>(sec->get_type()),
           static_cast<unsigned>(sec->get_memory_size()));
}

bool Converter::process(std::vector<u8>& buf) {
  bin::Header header;

  constexpr auto header_size = roundUp(sizeof(bin::Header), 32);

  BinaryBuilder builder(buf);

  builder.reserve(header_size);

  std::vector<std::size_t> section_offsets;
  u32 max_align = 32;
  if (!collectSections(section_offsets, max_align, buf))
    return false;

  builder.align(32);
  fillInCodeSection(
      header, header_size,
      std::span<u8>{buf.data() + header_size, buf.data() + buf.size()},
      max_align);

  auto kxMain = lookup("_start");
  if (!kxMain.has_value()) {
    kxMain = lookup("__start");
  }
  if (kxMain.has_value()) {
    // Section start relative to code start
    const auto section_start = section_offsets[kxMain->section] - header_size;
    header.entry_point_offset =
        static_cast<u32>(section_start + kxMain->offset);
  } else {
    printf("Cannot find entrypoint.\n");
    //  return false;
  }

  // Get the main symtab
  std::optional<ELFIO::symbol_section_accessor> symbols = findSymbolTable();

  if (!symbols.has_value()) {
    printf("Cannot find symbol table\n");
    return false;
  }

  RelocationExtractor extractor(*symbols);
  extractor.setRemapper([&](RelocationExtractor::MapEntry entry,
                            const std::string* symbol)
                            -> std::optional<RelocationExtractor::MapEntry> {
    // If the section is SHT_NULL, it's an extern
    if (mElf.sections[entry.section]->get_type() == SHT_NULL) {
      assert(symbol != nullptr);

      if (auto symbol_explicit = getSymbol(*symbol);
          symbol_explicit.has_value()) {
        // This will be applied statically
        return RelocationExtractor::MapEntry{
            .section = 0xFE, .offset = *symbol_explicit, .name = *symbol};
      }

      return RelocationExtractor::MapEntry{
          .section = 0xFF, .offset = addExtern(*symbol), .name = *symbol};
    }

    // We only have one section (a CODE section)
    if (section_offsets[entry.section] == ~0u) {
      printf("Invalid relocation against section %u: %s\n",
             (unsigned)entry.section,
             symbol ? symbol->c_str() : "NO SYMBOLNAME");
      return std::nullopt;
    }
    // assert(section_offsets[entry.section] != ~0u);
    const auto section_start = section_offsets[entry.section] - header_size;
    return RelocationExtractor::MapEntry{
        .section = 0, .offset = static_cast<u32>(section_start + entry.offset)};
  });

  if (!extractor.processRelocations(mElf))
    return false;

  auto& relocs = extractor.getRelocations();
  for (auto& reloc : relocs) {
    if (reloc.source_section != 0xFE)
      continue;
    // Apply the relocation now
    assert(reloc.affected_section == 0);
    const auto section_start = header_size;
    u8* affected = buf.data() + section_start + reloc.affected_offset;
    u32 source = reloc.source_offset + reloc.source_addend;
    switch (reloc.r_type) {
    case R_PPC_NONE:
      break;
    default:
      // 0xFD must still be resolved
      // The others can be done on link time though
      reloc.source_section = 0xFD;
      break;
    case R_PPC_ADDR32: {
      *reinterpret_cast<u32*>(affected) = swap32(source);
      break;
    }
    case R_PPC_ADDR24: {
      u32 old = swap32(*reinterpret_cast<u32*>(affected));
      old = (old & ~0x03fffffc) | (source & 0x03fffffc);
      *reinterpret_cast<u32*>(affected) = swap32(old);
      break;
    }
    case R_PPC_ADDR16: // Identical..
    case R_PPC_ADDR16_LO: {
      *reinterpret_cast<u16*>(affected) = swap16(source);
      break;
    }
    case R_PPC_ADDR16_HI: {
      *reinterpret_cast<u16*>(affected) = swap16(source >> 16);
      break;
    }
    case R_PPC_ADDR16_HA: {
      *reinterpret_cast<u16*>(affected) =
          swap16((source >> 16) + !!(source & 0x8000));
      break;
    }
    case R_PPC_ADDR14:
    case R_PPC_ADDR14_BRTAKEN:
    case R_PPC_ADDR14_BRNKTAKEN: {
      u32 old = swap32(*reinterpret_cast<u32*>(affected));
      old = (old & ~0x0000'fffc) | (source & 0x0000'fffc);
      *reinterpret_cast<u32*>(affected) = swap32(old);
      break;
    }
    }
  }

  std::remove_if(relocs.begin(), relocs.end(),
                 [](auto& reloc) { return reloc.source_section == 0xFE; });

  builder.writeRelocationsSection(header, extractor.getRelocations());
  builder.writeImportsSection(header, mImports);
  buf.resize(roundUp(buf.size(), 32));

  auto hdr = writeHeader(header);
  buf.erase(buf.begin(), buf.begin() + header_size);
  buf.insert(buf.begin(), hdr.begin(), hdr.end());

  return true;
}

std::optional<ELFIO::symbol_section_accessor> Converter::findSymbolTable() {
  for (auto& section : mElf.sections) {
    if (section->get_type() != SHT_SYMTAB)
      continue;

    return ELFIO::symbol_section_accessor(mElf, section);
  }

  return std::nullopt;
}

void Converter::fillInCodeSection(kx::bin::Header& header, u32 file_offset,
                                  std::span<u8> data, u32 align) {
  header.code.file_offset = file_offset;
  header.code.file_size = static_cast<u32>(data.size());
  header.code.flags = 0;
  header.code.reserved = 0;
  header.code.crc32 = crc32(data);
  header.code.alignment = align;
}

bool Converter::collectSections(std::vector<std::size_t>& section_offsets,
                                u32& max_align, std::vector<u8>& buf) {
  for (int s_index = 0; s_index < mElf.sections.size(); ++s_index) {
    const auto& section = *mElf.sections[s_index];

    if (!keep_section(section)) {
      section_offsets.push_back(~0u);
      continue;
    }

    // For now, everything is part of `code`
    // In the future, rodata/data/etc -> `data`
    // SHT_NOBITS -> `bss`
    u32 align = static_cast<u32>(section.get_addr_align());
    max_align = std::max(max_align, align);

    buf.resize(roundUp(buf.size(), align));
    const auto section_offset = buf.size();
    section_offsets.push_back(section_offset);
    if (section.get_type() == SHT_PROGBITS) {
      buf.insert(buf.end(), section.get_data(),
                 section.get_data() + section.get_size());
    } else if (section.get_type() == SHT_NOBITS) {
      buf.resize(buf.size() + section.get_size());
    } else {
      printf("Invalid section..\n");
      return false;
    }

    if (section.get_name() == ".ctors") {
      constexpr auto header_size = roundUp(sizeof(bin::Header), 32);
      mExplicitSymbols["__ctor_loc"] = section_offset - header_size;
      mExplicitSymbols["__ctor_end"] = buf.size() - header_size;
    }
  }

  return true;
}

std::optional<Converter::SymbolValue>
Converter::lookup(ELFIO::section& section, const std::string& key) {
  ELFIO::symbol_section_accessor symbols(mElf, &section);

  for (int i = 0; i < symbols.get_symbols_num(); ++i) {
    std::string name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind;
    unsigned char type;
    ELFIO::Elf_Half section_index;
    unsigned char other;
    if (!symbols.get_symbol(i, name, value, size, bind, type, section_index,
                            other))
      continue;

#ifndef NDEBUG
      // std::cout << name << std::endl;
#endif

    if (name != key)
      continue;

    return SymbolValue{section_index, static_cast<int>(value)};
  }

  return std::nullopt;
}

std::optional<Converter::SymbolValue>
Converter::lookup(const std::string& name) {
  for (auto& section : mElf.sections) {
    if (section->get_type() == SHT_SYMTAB) {
#ifndef NDEBUG
      printf("---\n");
#endif
      if (auto val = lookup(*section, name); val.has_value()) {
        return val;
      }
    }
  }

  return std::nullopt;
}

u32 Converter::addExtern(std::string_view string) {
  if (auto it = std::find(mImports.begin(), mImports.end(), string);
      it != mImports.end()) {
    return static_cast<u32>(std::distance(mImports.begin(), it));
  }

  mImports.emplace_back(string);
  return static_cast<u32>(mImports.size() - 1);
}

bool Converter::keep_section(const ELFIO::section& section) {
  if (section.get_name() == ".eh_frame")
    return false;
  if (section.get_name() == ".comment")
    return false;

  return section.get_type() == SHT_PROGBITS || section.get_type() == SHT_NOBITS;
}

std::optional<u32> Converter::getSymbol(std::string_view string) const {
  std::string key{string};

  auto it = mExplicitSymbols.find(key);
  if (it == mExplicitSymbols.end())
    return std::nullopt;

  return {it->second};
}

} // namespace kx
