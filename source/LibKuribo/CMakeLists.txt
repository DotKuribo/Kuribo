project(LibKuribo)

set(CMAKE_CXX_STANDARD 17)

add_library(LibKuribo
	"modules/kxer/Loader.hxx"
	"modules/kxer/Loader.cxx"
	"modules/kxer/Module.hxx"	
	"modules/kxer/Module.cxx"
	"modules/Project.hxx"
	"modules/Project.cxx"
	"modules/SymbolManager.hxx"
	"modules/SymbolManager.cxx"
	"modules/SymbolManager.cxx"
	"modules/SymbolManager.cxx"
	"filesystem.hxx"
	"filesystem.cxx"
)

target_include_directories(LibKuribo PUBLIC
	"./"
	"../"
	"${PROJECT_SOURCE_DIR}/../Common"
	"${PROJECT_SOURCE_DIR}/../Common/vendor"
	"${PROJECT_SOURCE_DIR}/../"
	"${PROJECT_SOURCE_DIR}/../../"
)