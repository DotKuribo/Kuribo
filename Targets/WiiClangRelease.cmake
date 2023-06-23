set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR powerpc)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(triple powerpc-unknown-eabi)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(WII_DEFINES "-O3 -DGEKKO -DKURIBO_PLATFORM_WII=1 -fno-exceptions -fno-rtti -ffast-math -fpermissive -D__powerpc__ -DEA_PLATFORM_LINUX --target=ppc-linux-eabi -mcpu=750 -DEA_COMPILER_CPP17_ENABLED -DEA_COMPILER_CPP14_ENABLED -DEA_COMPILER_CPP11_ENABLED -DEA_HAVE_CPP11_MUTEX ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WII_DEFINES}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${WII_DEFINES}")

set(WII_LINK " -flto -nostdlib -fuse-ld=lld  -mllvm --march=ppc-linux-eabi -mllvm -mcpu=750 ")

set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${WII_LINK}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${WII_LINK}")


set(LIBSTDCPP_VERSION "12.1.0")
set(DKP_PATH "C:/devkitPro/devkitPPC/powerpc-eabi/include")

include_directories(SYSTEM
	"C:/devkitPro/devkitPPC/powerpc-eabi/include/c++/${LIBSTDCPP_VERSION}"
	"C:/devkitPro/devkitPPC/powerpc-eabi/include"
	"C:/devkitPro/devkitPPC/powerpc-eabi/include/c++/${LIBSTDCPP_VERSION}/powerpc-eabi"
)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

set(CMAKE_OBJCOPY C:/devkitPro/devkitPPC/bin/powerpc-eabi-objcopy.exe CACHE PATH "" FORCE)