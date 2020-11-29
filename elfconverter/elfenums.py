class ELFFlags:
    class SymBind:
        STB_LOCAL = 0
        STB_GLOBAL = 1
        STB_WEAK = 2
        STB_LOPROC = 13
        STB_HIPROC = 15

    class SymType:
        STT_NOTYPE = 0
        STT_OBJECT = 1
        STT_FUNC = 2
        STT_SECTION = 3
        STT_FILE = 4
        STT_LOPROC = 13
        STT_HIPROC = 15

    class Reloc:
        R_PPC_ADDR32 = 1
        R_PPC_ADDR16_LO = 4
        R_PPC_ADDR16_HI = 5
        R_PPC_ADDR16_HA = 6
        R_PPC_REL24 = 10