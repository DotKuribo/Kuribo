set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR powerpc)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(triple powerpc-unknown-eabi)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(WII_DEFINES "-O1 -fno-inline -DKURIBO_PLATFORM_WII=1 -fno-exceptions -fno-rtti -fdata-sections -ffunction-sections -flto -ffast-math -fpermissive -D__powerpc__ -DEA_PLATFORM_LINUX --target=ppc-linux-eabi -mcpu=750 -DEA_COMPILER_CPP17_ENABLED -DEA_COMPILER_CPP14_ENABLED -DEA_COMPILER_CPP11_ENABLED -DEA_HAVE_CPP11_MUTEX ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WII_DEFINES}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${WII_DEFINES}")

set(WII_LINK " -nostdlib -fuse-ld=lld -Wl,--gc-sections -mllvm --march=ppc-linux-eabi -mllvm -mcpu=750 ")

set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${WII_LINK}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${WII_LINK}")


set(LIBSTDCPP_VERSION "10.2.0")
set(DKP_PATH "C:/devkitPro/devkitPPC/powerpc-eabi/include")

include_directories(SYSTEM
	"C:/devkitPro/devkitPPC/powerpc-eabi/include/c++/10.2.0"
	"C:/devkitPro/devkitPPC/powerpc-eabi/include"
	"C:/devkitPro/devkitPPC/powerpc-eabi/include/c++/10.2.0/powerpc-eabi"
)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

set(CMAKE_OBJCOPY C:/devkitPro/devkitPPC/bin/powerpc-eabi-objcopy.exe CACHE PATH "" FORCE)