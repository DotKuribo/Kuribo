#include "StringTable.hpp"
#include "format/Binary.hpp"

namespace kx {

std::size_t StringTable::calcTableSize() const {
  return mStrings.size() * sizeof(bin::BinaryString);
}

std::size_t StringTable::calcPoolSize() const {
  std::size_t string_size = 0;

  for (auto& str : mStrings)
    string_size += str.length() + 1;

  return string_size;
}

} // namespace kx