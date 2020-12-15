# CMakeList.txt : CMake project for KuriboConverter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("KuriboConverter")

set(CMAKE_CXX_STANDARD 20)

# Add source to this project's executable.
add_executable (KuriboConverter
	"KuriboConverter.cpp"
	
	"source/format/Binary.hpp"
	"source/types.h"
	"source/converter/Converter.hpp"
	"source/converter/Converter.cpp"
	"source/crc.hpp"
	"source/converter/RelocationExtractor.hpp"
	"source/converter/RelocationExtractor.cpp"
	"source/converter/BinaryBuilder.hpp"
	"source/converter/BinaryBuilder.cpp"
	"source/converter/StringTable.hpp"
	"source/converter/StringTable.cpp"
)

target_include_directories(KuriboConverter PRIVATE "source" "vendor")

# TODO: Add tests and install targets if needed.
