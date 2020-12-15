# Kuribo
Minecraft Forge for the Wii (WIP)

## Upcoming features
- Channel scripting, custom GUI, more..

## Objective
Kuribo is a Wii Channel and standalone binary (for use in ROM hacks).
Kuribo loads mods from the `mods` folder on your SD card, allowing drag-and-drop installation.

## Supported mod formats
- .kxe: Kuribo executable
	- Supports any compiler: GCC, Clang, and CodeWarrior.
	- Supports compression, encryption, linking across mods.
	- Supports embedded romfs
- .kmk: Kamek executables (legacy support)
	- Only works with CodeWarrior (ANSI C/C++98)

## Gecko Support
Gecko codes are run through Kuribo's GeckoJIT engine. GeckoJIT is the fastest code handler, by a considerable margin:

### Runtime comparison
Averaged across a minute of data (3600 samples). Tested on the first quarter of the FKW .gct for PAL MKW. This was necessary to support the USB Gecko codehandler, which has an extremely limited hard restriction on code length. The source code of the instrumentation code can be found [here](https://github.com/riidefi/Kuribo/blob/master/GeckoJIT/tests.cxx#L513-L554). A cheat code version is available [here](https://mkwii.com/showthread.php?tid=1673&pid=6040#pid6040), although its measurements will be a bit biased (in favor of the codehandler), as the cache will be primed when the test occurs (as it is itself a code).
<p align="center">
  <img src="https://raw.githubusercontent.com/riidefi/Kuribo/master/Docs/gecko_comparison.png">
</p>

### Supported formats
- .gct: Standard binary format
- .txt: Text version of codes. Kuribo can convert these to a GCT for you.

## Writing a module
Here is an example Kuribo module. (C++)
```cpp
// Make sure to add the /sdk/ folder to your include path
#include "kuribo_sdk.h"

KURIBO_MODULE_BEGIN("DemoModule", "riidefi", "Beta")
{
	KURIBO_EXECUTE_ON_LOAD { OSReport("Loading DemoModule\n"); }
	KURIBO_EXECUTE_ON_UNLOAD { OSReport("Unloading DemoModule!\n"); }
}
KURIBO_MODULE_END();
```
1. Compile your .cpp files: `g++ DemoModule.cpp -o DemoModule.elf -IKuribo/sdk`
2. Build a .kxe file: `KuriboConverter.exe DemoModule.elf DemoModule.kxe`
3. You're done! Add to your mods folder and boot Kuribo.

When we launch the game, we see:
```
[OSREPORT]: ~~~~~~~~~~~~~~~~~~~~~~~
[OSREPORT]: [KURIBO] Loading module
[OSREPORT]:          Name:     	DemoModule
[OSREPORT]:          Author:   	riidefi
[OSREPORT]:          Version:  	Beta
[OSREPORT]:                      
[OSREPORT]:          Built:    	Dec 11 2020 at 13:27:54
[OSREPORT]:          Compiler: 	GCC 10.2.0
[OSREPORT]: ~~~~~~~~~~~~~~~~~~~~~~~
[OSREPORT]: Loading DemoModule!
```

### Patching game functions
Between our MODULE_BEGIN and MODULE_END block, we have the following APIs:
- KURIBO_PATCH_B(addr, value)
- KURIBO_PATCH_BL(addr, value)
- KURIBO_PATCH_32(addr, value)

Example:
```cpp
#include "kuribo_sdk.h"

static void MyGetBMGID(u32 /*track_id*/) {
	return 0x245C;
}

KURIBO_MODULE_BEGIN("DemoModule", "riidefi", "Beta")
{
	KURIBO_EXECUTE_ON_LOAD { OSReport("Loading DemoModule\n"); }
	KURIBO_EXECUTE_ON_UNLOAD { OSReport("Unloading DemoModule!\n"); }

	KURIBO_PATCH_B(0x80833668, MyGetBMGID);
}
KURIBO_MODULE_END();
```
When compiling as C++11, we can also use the following form:
```cpp
#include "kuribo_sdk.h"

KURIBO_MODULE_BEGIN("DemoModule", "riidefi", "Beta")
{
	KURIBO_EXECUTE_ON_LOAD { OSReport("Loading DemoModule\n"); }
	KURIBO_EXECUTE_ON_UNLOAD { OSReport("Unloading DemoModule!\n"); }

	kBranch(0x80833668, { return 0x245C; });
}
KURIBO_MODULE_END();
```

### Sharing functions across modules:
By using KURIBO_EXPORT, functions from one module can be made available to others. 
```cpp
KURIBO_EXPORT(MyFunction);
```
For example, you could put all your shared logic in `MyLibrary.kxe` and have two codes `MyItemRain.kxe` and `My200cc.kxe` both use `MyLibrary.kxe`

### Accessing Kuribo APIs
The following code accesses the GeckoJIT API:
```cpp
KURIBO_EXECUTE_ON_LOAD {
	static char my_heap[1024];

	static const u32 my_code[2] = {
		// Officially cancel THP
		0x04552C28, 0x60000000
	};

	kxCompiledFunction my_func = kxGeckoJitCompileCodes(
		my_heap, sizeof(my_heap),
		my_code, sizeof(my_code)
	);

	if (my_func != nullptr) {
		// NOTE: We call this code once. Many codes need to be ran
		//       every frame.
		my_func();
	} else {
		OSReport("Failed to compile my_func :{\n");
	}
}
```