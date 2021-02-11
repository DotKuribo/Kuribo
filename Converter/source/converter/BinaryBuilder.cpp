#include "BinaryBuilder.hpp"
#include "StringTable.hpp"
#include "crc.hpp"
#include <string.h>

namespace kx {

void flip(u32& v) { v = swap32(v); }
void flip(u16& v) { v = swap16(v); }
void flip(u8) {}

void flip(bin::Section& sec) {
  flip(sec.file_offset);
  flip(sec.file_size);
  flip(sec.crc32);
  flip(sec.alignment);
  flip(sec.flags);
  flip(sec.reserved);
}
void flip(bin::Header& header) {
  flip(header.magic);
  flip(header.kmxe_version);
  flip(header.kernel_version);
  flip(header.file_size);
  flip(header.flags);
  flip(header.code);
  flip(header.data);
  flip(header.bss);
  flip(header.relocations);
  flip(header.imports);
  flip(header.exports);
  flip(header.entry_point_offset);
  flip(header.embedded_file);
}

std::vector<u8> writeHeader(const bin::Header& in_header) {
  std::vector<u8> mData(roundUp(sizeof(bin::Header), 32));

  bin::Header header = in_header;
  flip(header);

  memcpy(mData.data(), &header, sizeof(header));

  return mData;
}

void BinaryBuilder::writeImportsSection(
    kx::bin::Header& header, const std::vector<std::string>& imports) {
  mData.resize(roundUp(mData.size(), 32));
  const auto import_offset = mData.size();

  header.imports.file_offset = static_cast<u32>(import_offset);

  header.imports.alignment = 32;
  header.imports.crc32 = 0;
  header.imports.flags = 0;
  header.imports.reserved = 0;

  packStringTable(mData, imports);

  header.imports.file_size = static_cast<u32>(mData.size() - import_offset);
}

void BinaryBuilder::writeRelocationsSection(
    kx::bin::Header& header, const std::vector<Relocation>& relocations) {
  auto write_u32 = [&](u32 val) {
    mData.push_back(val >> 24);
    mData.push_back((val << 8) >> 24);
    mData.push_back((val << 16) >> 24);
    mData.push_back((val << 24) >> 24);
  };
  mData.resize(roundUp(mData.size(), 32));
  const auto relocation_offset = mData.size();

  header.relocations.file_offset = static_cast<u32>(relocation_offset);
  header.relocations.alignment = 32;
  header.relocations.crc32 = 0;
  header.relocations.flags = 0;
  header.relocations.reserved = 0;

  for (auto& reloc : relocations) {
    mData.push_back(reloc.r_type);
    mData.push_back(reloc.affected_section);
    mData.push_back(reloc.source_section);
    mData.push_back(reloc.pad);

    write_u32(reloc.affected_offset);
    write_u32(reloc.source_offset);
    write_u32(reloc.source_addend);
  }

  header.relocations.file_size =
      static_cast<u32>(mData.size() - relocation_offset);
}

void BinaryBuilder::packStringTable(std::vector<u8>& buf,
                                    const std::vector<std::string>& strings) {
  StringTable table(strings);

  const u32 section_start = static_cast<u32>(buf.size());
  writeStringTable(section_start, table, strings, buf);
  writeStringPool(strings, buf);
}

void BinaryBuilder::writeStringPool(const std::vector<std::string>& strings,
                                    std::vector<u8>& buf) {
  for (auto& str : strings) {
    for (auto c : str)
      buf.push_back(c);
    buf.push_back(0);
  }
}

void BinaryBuilder::writeStringTable(u32 section_start, kx::StringTable& table,
                                     const std::vector<std::string>& strings,
                                     std::vector<u8>& buf) {
  const u32 string_start =
      section_start + static_cast<u32>(table.calcTableSize());

  u32 counter = 0;
  for (auto& str : strings) {
    auto write_u32 = [&](u32 val) {
      buf.push_back(val >> 24);
      buf.push_back((val << 8) >> 24);
      buf.push_back((val << 16) >> 24);
      buf.push_back((val << 24) >> 24);
    };
    write_u32(string_start + counter);
    write_u32(static_cast<u32>(str.length()));
    write_u32(crc32(str));
    counter += static_cast<u32>(str.length() + 1);
  }
}

} // namespace kx