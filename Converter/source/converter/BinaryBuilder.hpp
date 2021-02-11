#pragma once

#include "RelocationExtractor.hpp"
#include "StringTable.hpp"
#include <format/Binary.hpp>
#include <types.h>
#include <vector>

namespace kx {

std::vector<u8> writeHeader(const bin::Header& in_header);

class BinaryBuilder {
public:
  BinaryBuilder(std::vector<u8>& data) : mData(data) {}

  std::size_t align(u32 align) {
    mData.resize(roundUp(static_cast<u32>(mData.size()), align));
    return mData.size();
  }

  std::size_t reserve(u32 size) {
    mData.resize(static_cast<u32>(mData.size()) + size);
    return mData.size();
  }

  void writeImportsSection(kx::bin::Header& header,
                           const std::vector<std::string>& imports);

  void writeRelocationsSection(kx::bin::Header& header,
                               const std::vector<Relocation>& relocations);

  void packStringTable(std::vector<u8>& buf,
                       const std::vector<std::string>& strings);

  void writeStringPool(const std::vector<std::string>& strings,
                       std::vector<u8>& buf);

  void writeStringTable(u32 section_start, kx::StringTable& table,
                        const std::vector<std::string>& strings,
                        std::vector<u8>& buf);

private:
  std::vector<u8>& mData;
};

} // namespace kx