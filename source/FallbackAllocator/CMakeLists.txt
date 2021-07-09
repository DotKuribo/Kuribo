project(GeckoJIT)

set(CMAKE_CXX_STANDARD 20)

add_library(FallbackAllocator
	"xalloc/Allocator.h"
	"xalloc/FreeListAllocator.h"
	"xalloc/SinglyLinkedList.h"
	"xalloc/SinglyLinkedListImpl.h"
	"xalloc/Utils.h"
	"xalloc/FreeListAllocator.cxx"
	"free_list_heap.hxx"
)

target_include_directories(FallbackAllocator PUBLIC
	"./"
	"${PROJECT_SOURCE_DIR}/../"	
	"${PROJECT_SOURCE_DIR}/../Common"
	"${PROJECT_SOURCE_DIR}/../Common/vendor"
	)