#pragma once

#include "elfio/elfio.hpp"
#include "format/Binary.hpp"
#include <optional>
#include <vector>
#include <functional>

namespace kx {

class RelocationExtractor {
public:
  RelocationExtractor(ELFIO::symbol_section_accessor symbols)
      : mSymbols(symbols) {}
  bool processRelocations(const ELFIO::elfio& elf);
  bool processRelocationSection(ELFIO::relocation_section_accessor section,
                                u32 affected_section);

  struct MapEntry {
    u8 section;
    u32 offset;
  };

  using Remapper = std::function<MapEntry(MapEntry, const std::string*)>;
  void setRemapper(Remapper remapper) { mRemapper = remapper; }

  const std::vector<bin::Relocation>& getRelocations() const { return mRelocations; }

private:
  Remapper mRemapper = nullptr;
  ELFIO::symbol_section_accessor mSymbols;
  std::vector<bin::Relocation> mRelocations;
};

} // namespace kx