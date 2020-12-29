cmake_minimum_required (VERSION 3.8)

project ("KuriboKernel")

set(CMAKE_CXX_STANDARD 20)

add_executable (KuriboKernel
	
	"source/io/filesystem.hxx"
	"source/io/io.hxx"
	"source/modules/kamek/Loader.hxx"
	"source/modules/kamek/Module.hxx"
	"source/modules/kamek/kamek.h"
	"source/system/memory.hxx"
	"source/system/memory/free_list_heap.hxx"
	"../Common/memory/heap.hxx"
	"source/system/memory/stack_alloc.hxx"
	"source/system/memory/xalloc/Allocator.h"
	"source/system/memory/xalloc/FreeListAllocator.h"
	"source/system/memory/xalloc/SinglyLinkedList.h"
	"source/system/memory/xalloc/SinglyLinkedListImpl.h"
	"source/system/memory/xalloc/Utils.h"
	"source/system/system.hxx"
	
	"source/entry.cxx"
	"source/io/filesystem.cxx"
	"source/modules/gecko/Frontend.cxx"
	"source/modules/kamek/Loader.cxx"
	"source/platform.cxx"
	"source/system/memory/memory.cxx"
	"source/system/memory/xalloc/FreeListAllocator.cxx"
	"source/system/system.cxx"
	"source/test.cxx"

	"source/vendor/libc.c"
)

target_include_directories(KuriboKernel PRIVATE "source" "source/vendor" "../" "../sdk" "../Common" "../Common/vendor" "../LibKuribo")

include_directories(${PROJECT_SOURCE_DIR}/../Common)

if (KURIBO_BUILD_WII)
	set(KERNEL_LINK, " -Wl,--script=${CMAKE_SOURCE_DIR}/arch/gc.lcf ")

	set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--script=${CMAKE_SOURCE_DIR}/Kernel/arch/gc.lcf ")
	set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,--script=${CMAKE_SOURCE_DIR}/Kernel/arch/gc.lcf ")

	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(KERNEL_COMPILE "-fsanitize=cfi -fvisibility=hidden")

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${KERNEL_COMPILE}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KERNEL_COMPILE}")

		target_link_libraries(KuriboKernel Common GeckoJIT "${CMAKE_SOURCE_DIR}/Targets/libKuriboClang.a")
	else()
		set(KERNEL_COMPILE "-Wno-multichar")

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${KERNEL_COMPILE}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KERNEL_COMPILE}")

		target_link_libraries(KuriboKernel Common GeckoJIT LibKuribo)
	endif()

	add_custom_target(${PROJECT_NAME}.bin ALL DEPENDS ${PROJECT_NAME})
	add_custom_command(
		TARGET ${PROJECT_NAME}.bin
		COMMAND ${CMAKE_OBJCOPY} ARGS -O binary ${PROJECT_NAME} ${PROJECT_NAME}.bin
	)

	if (DEFINED ENV{KURIBO_KERNEL_DEPLOY_PATH})
		add_custom_target(BOTH ALL DEPENDS ${PROJECT_NAME})
		add_custom_command(
			TARGET BOTH
			COMMAND ${CMAKE_COMMAND} -E copy
					${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
					$ENV{KURIBO_KERNEL_DEPLOY_PATH})
	else()
		message("Deploy path is not set")
	endif()
else()
	target_link_libraries(KuriboKernel PUBLIC
		Common
		GeckoJIT
		LibKuribo
	)
endif()