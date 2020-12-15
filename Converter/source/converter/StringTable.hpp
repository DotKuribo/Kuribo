#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace kx {

class StringTable {
public:
  StringTable() = default;
  StringTable(const std::vector<std::string>& strings) : mStrings(strings) {}

  std::size_t calcTableSize() const;
  std::size_t calcPoolSize() const;
  std::size_t calcTotalSize() const { return calcTableSize() + calcPoolSize(); }


private:
  std::vector<std::string> mStrings;
};

} // namespace kx