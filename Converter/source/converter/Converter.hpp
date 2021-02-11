#pragma once

#include "elfio/elfio.hpp"
#include "format/Binary.hpp"
#include <map>
#include <optional>
#include <set>
#include <span>
#include <types.h>

namespace kx {

class Converter {
public:
  Converter() = default;
  ~Converter() = default;

  void readSymbols(const std::string& path);
  bool addElf(const char* path);
  void printElfInfo(const ELFIO::elfio& elf) const;
  bool process(std::vector<u8>& buf);

private:
  std::optional<ELFIO::symbol_section_accessor> findSymbolTable();
  void fillInCodeSection(kx::bin::Header& header, u32 file_offset,
                         std::span<u8> data, u32 align);
  bool collectSections(std::vector<std::size_t>& section_offsets,
                       u32& max_align, std::vector<u8>& buf);
  struct SymbolValue {
    int section;
    int offset; // Relative to section start
  };
  std::optional<SymbolValue> lookup(ELFIO::section& section,
                                    const std::string& name);
  std::optional<SymbolValue> lookup(const std::string& name);
  u32 addExtern(std::string_view string);
  bool keep_section(const ELFIO::section& section);

  // User specified externs
  std::optional<u32> getSymbol(std ::string_view string) const;

  ELFIO::elfio mElf;
  std::map<std::string, u32> mExplicitSymbols;
  std::vector<std::string> mImports;
};

} // namespace kx