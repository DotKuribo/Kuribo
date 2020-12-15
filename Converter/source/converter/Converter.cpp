#include "Converter.hpp"
#include "BinaryBuilder.hpp"
#include "RelocationExtractor.hpp"
#include "crc.hpp"
#include <assert.h>

namespace kx {

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

  if (auto kxMain = lookup("_start"); kxMain.has_value()) {
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
  extractor.setRemapper(
      [&](RelocationExtractor::MapEntry entry,
          const std::string* symbol) -> RelocationExtractor::MapEntry {
        // If the section is SHT_NULL, it's an extern
        if (mElf.sections[entry.section]->get_type() == SHT_NULL) {
          assert(symbol != nullptr);
          return {.section = 0xFF, .offset = addExtern(*symbol)};
        }

        // We only have one section (a CODE section)
        assert(section_offsets[entry.section] != ~0u);
        const auto section_start = section_offsets[entry.section] - header_size;
        return {.section = 0,
                .offset = static_cast<u32>(section_start + entry.offset)};
      });

  if (!extractor.processRelocations(mElf))
    return false;

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
    section_offsets.push_back(buf.size());
    if (section.get_type() == SHT_PROGBITS) {
      buf.insert(buf.end(), section.get_data(),
                 section.get_data() + section.get_size());
    } else if (section.get_type() == SHT_NOBITS) {
      buf.resize(buf.size() + section.get_size());
    } else {
      printf("Invalid section..\n");
      return false;
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
    std::cout << name << std::endl;
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

} // namespace kx
