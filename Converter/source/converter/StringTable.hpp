#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace kx {

//! A StringTable is composed of two sections: an array of `BinaryString`s
//! (offset, len, crc) and a pool of string data (with null terminators)
class StringTable {
public:
  StringTable() = default;
  StringTable(const std::vector<std::string>& strings) : mStrings(strings) {}

  // Array of `BinaryString`s
  std::size_t calcTableSize() const;

  // Pool of character data delimited by null characters
  std::size_t calcPoolSize() const;

  // Total data size
  std::size_t calcTotalSize() const { return calcTableSize() + calcPoolSize(); }

private:
  std::vector<std::string> mStrings;
};

} // namespace kx