#pragma once

#include "Frontend.hxx"
#include "common.h"

namespace kuribo::gecko {

class CodePrinter final {
public:
  void onCodeBegin(eastl::string_view title) {
    eastl::string to_print(title.begin(), title.size());
    KURIBO_LOG("\nCode: %s\n=====\n", to_print.c_str());
  }
  void onCodeChunk(u32 chunk) {
    KURIBO_LOG_FUNCTION("%08x%c", chunk, odd ? ' ' : '\n');
    odd = !odd;
  }
  void onParseFail() {}
  void onEnd() {}

private:
  bool odd = true;
};

} // namespace kuribo::gecko