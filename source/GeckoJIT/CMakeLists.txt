project(GeckoJIT)

set(CMAKE_CXX_STANDARD 20)

add_executable(GeckoJIT
	"allocator/frame.hpp"
	"engine/compiler.hpp"
	"engine/engine.hpp"
	"engine/compiler.cxx"
	"engine/engine.cxx"
	"tests.cxx"
	"module.cxx"
)

target_link_libraries(GeckoJIT FallbackAllocator)

set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-r ")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-r ")

# TODO: -r with -Wl,--gc-sections

target_include_directories(GeckoJIT PUBLIC "./" "${PROJECT_SOURCE_DIR}/../Common" "${PROJECT_SOURCE_DIR}/../Common/vendor" "../../sdk/")

add_custom_target(${PROJECT_NAME}.kxe ALL DEPENDS ${PROJECT_NAME})
# TODO: This involves manually updating KuriboConverter.exe (from Converter project output)
add_custom_command(
	TARGET ${PROJECT_NAME}.kxe
	COMMAND ${PROJECT_SOURCE_DIR}/../../tools/KuriboConverter.exe ARGS ${PROJECT_NAME} ${PROJECT_NAME}.kxe f
)

if (DEFINED ENV{KURIBO_KERNEL_DEPLOY_PATH})
	add_custom_target(GeckoJIT_KXEDeploy ALL DEPENDS ${PROJECT_NAME})
	add_custom_command(
		TARGET GeckoJIT_KXEDeploy
		COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.kxe
				$ENV{KURIBO_KERNEL_DEPLOY_PATH}/../Mods/GeckoJIT.kxe)
else()
	message("Deploy path is not set")
endif()