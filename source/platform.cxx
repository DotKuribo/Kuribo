#include "api/HostInterop.h"

/* Defines the function addresses in PAL MKW */
#define OSReport 0x801A25D0
#define OSFatal 0x801A4EC4
#define DVDConvertPathToEntryNum 0x8015DF4C
#define DVDFastOpen 0x8015E254
#define DVDReadPrio 0x8015E834
#define DVDClose 0x8015E568

/* Sets GC/Wii OS functions. */
KURIBO_SET_OS(OSReport, OSFatal)

/* Sets functions to read from the disc. */
KURIBO_SET_DVD(DVDConvertPathToEntryNum, DVDFastOpen, DVDReadPrio, DVDClose)
