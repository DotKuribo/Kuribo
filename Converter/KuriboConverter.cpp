#include "converter/Converter.hpp"
#include "format/Binary.hpp"
#include <iostream>

void writeFile(const std::span<uint8_t> data, const std::string_view path) {
  std::ofstream stream(std::string(path), std::ios::binary | std::ios::out);
  stream.write(reinterpret_cast<const char*>(data.data()), data.size());
}

int main(int argc, char** argv) {
  printf("--------------------------------------\n");
  printf("KuriboConverter v1.0 by riidefi\n");
  printf(" Compatible with kernel version %u\n", KURIBO_CORE_VERSION);
  printf("--------------------------------------\n");

  const bool is_help_wanted =
      argc >= 2 && (!strcmp(argv[1], "-help") || !strcmp(argv[1], "-help"));

  if (argc < 2 || is_help_wanted) {
    fprintf(stderr, "Usage: KuriboConverter.exe <source.elf> [dest.kxe]");
    return 1;
  }

#ifdef INTERNAL

#else
  const std::string source_path = argv[1];
  const std::string dest_path = [&]() -> std::string {
    if (argc == 2) {
      std::string tmp = source_path;
      auto search = tmp.rfind(".elf");
      if (search != std::string::npos)
        tmp.replace(search, sizeof(".elf") - 1, ".kxe");
      else
        tmp.append(".kxe");
      return tmp;
    } else {
      return argv[2];
    }
  }();
#endif
  kx::Converter converter;

  if (!converter.addElf(source_path.c_str())) {
    printf("Not a valid 32-bit ELF file\n");
    return 1;
  }

  std::vector<u8> buf;
  if (!converter.process(buf)) {
    printf("Failed to process ELF\n");
    return 1;
  }

  writeFile(buf, dest_path);
  return 0;
}
