# CMakeList.txt : CMake project for KuriboConverter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("Kuribo")

add_subdirectory(source/Common)
add_subdirectory(source/GeckoJIT)
add_subdirectory(source/LibKuribo)
add_subdirectory(source/FallbackAllocator)
add_subdirectory(source/Kernel)
add_subdirectory(examples)
