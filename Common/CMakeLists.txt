project(Common)

add_library(Common
	"api/HostInterop.h"
	"api/Module.h"
	"common.h"
	"config.h"
	"types.h"
	"core/patch.hxx"
	"core/sync.hxx"
	"debug.h"
	"debug/assert.h"
	"debug/log.h"
	"util/crc.hxx"
	"util/deferred.hxx"
	"util/json.hxx"
	"debug/kassert.cxx"
)

target_include_directories(Common PUBLIC "./")