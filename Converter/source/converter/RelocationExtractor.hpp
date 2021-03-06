#pragma once

#include "elfio/elfio.hpp"
#include "format/Binary.hpp"
#include <assert.h>
#include <functional>
#include <optional>
#include <vector>

namespace kx {

struct Relocation : public bin::Relocation {
  Relocation() = default;
  Relocation(bin::Relocation r) {
    assert((r.affected_offset & 0x8000'0000) == 0);

    static_cast<bin::Relocation&>(*this) = r;
  }
  // DEBUG
  std::string affected_symbol;
  std::string source_symbol;
};

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
    // DEBUG
    std::string name;
  };

  using Remapper =
      std::function<std::optional<MapEntry>(MapEntry, const std::string*)>;
  void setRemapper(Remapper remapper) { mRemapper = remapper; }

  std::vector<Relocation>& getRelocations() { return mRelocations; }

private:
  Remapper mRemapper = nullptr;
  ELFIO::symbol_section_accessor mSymbols;
  std::vector<Relocation> mRelocations;
};

} // namespace kx