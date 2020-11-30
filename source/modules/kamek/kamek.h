#pragma once

// Stripped down kamek header for loader

// hook type IDs _must_ match what's in the Kamek source!
#define kctWrite 1
#define kctConditionalWrite 2
#define kctInjectBranch 3
#define kctInjectCall 4
#define kctPatchExit 5

#define kmIdentifier(key, counter) _k##key##counter
