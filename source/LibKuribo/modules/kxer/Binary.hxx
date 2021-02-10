#pragma once

#include <EASTL/string_view.h>
#include <sdk/kuribo_sdk.h>
#include <types.h>

namespace kx::bin {

struct BinaryString {
  u32 offset;
  u32 length;
  u32 crc32;
};

inline eastl::string_view resolveBinaryString(const BinaryString& string,
                                              const void* file, u32 file_size) {
  if (string.offset + string.length > file_size)
    return "<Invalid String>";

  const char* begin = reinterpret_cast<const char*>(file) + string.offset;
  const u32 length = string.length;

  return eastl::string_view(begin, length);
}

struct MetadataV1 {
  BinaryString compiler_name;
  BinaryString build_date;

  // user-reported data
  BinaryString module_name;
  BinaryString module_author;
  BinaryString module_version;
};

enum class SectionFlags : u8 { CompressedSZS = (1 << 0) };

struct Section {
  u32 file_offset = ~0;
  u32 file_size = ~0;
  u32 crc32 = 0;

  u8 alignment = 32;
  u8 flags = 0; // SectionFlags
  u16 reserved = 0;
};

struct Header {
  u32 magic = 'KXER'; // 'K' 'X' 'E' 'R'(elocatable)

  // We probably won't change the magic unless it's a drastic change.
  // The following definitions are for version 0
  u16 kmxe_version = 0;

  // Compatible Kernel API version
  u16 kernel_version = KURIBO_CORE_VERSION;

  u32 file_size = 0;

  // Mostly reserved bits
  u32 flags = 0;

  Section code;
  Section data;
  Section bss; // Offset is 0

  // List of `Relocation`s
  Section relocations;
  // List of `BinaryString`s
  Section imports;
  //
  Section exports;

  // This is releative to the start of the CODE section
  u32 entry_point_offset = 0;

  // Embed an archive within the file to be accessed by the code itself.
  Section embedded_file;

  // MetadataV1 metadata;
};
enum { HEADER_SIZE = sizeof(Header) };

struct Relocation {
  u8 r_type; // R_PPC_*

  u8 affected_section; // 0 - code, 1 - data, 2 - bss, 0xFF extern (source only)
  u8 source_section;   // Usually in a data segment, setting an ADDI
  u8 pad;

  u32 affected_offset; // offset from start of above section
  u32 source_offset;
  u32 source_addend;
};

} // namespace kx::bin

enum PPCRelocations {
  R_PPC_NONE,
  R_PPC_ADDR32,
  R_PPC_ADDR24,
  R_PPC_ADDR16,
  R_PPC_ADDR16_LO,
  R_PPC_ADDR16_HI,
  R_PPC_ADDR16_HA,
  R_PPC_ADDR14,
  R_PPC_ADDR14_BRTAKEN,
  R_PPC_ADDR14_BRNKTAKEN,
  R_PPC_REL24,
  R_PPC_REL14,

  R_PPC_REL32 = 26
};
