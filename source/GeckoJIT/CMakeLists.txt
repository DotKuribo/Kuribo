project(GeckoJIT)

set(CMAKE_CXX_STANDARD 20)

add_library(GeckoJIT
	"allocator/frame.hpp"
	"engine/compiler.hpp"
	"engine/engine.hpp"
	"engine/compiler.cxx"
	"engine/engine.cxx"
	"tests.cxx"
)

target_include_directories(GeckoJIT PUBLIC "./" "${PROJECT_SOURCE_DIR}/../Common" "${PROJECT_SOURCE_DIR}/../Common/vendor")