from __future__ import annotations

from io import BytesIO
from elftools.elf.elffile import ELFFile, Section

from addressmapper import AddressMapper
from elfenums import ELFFlags
from exceptions import (AlreadyLinkedException,
                        AlreadyExistsException,
                        InvalidOperationException,
                        InvalidDataException,
                        InvalidTableLinkageException)
from ioreader import *
from kmhooks import HookData
from kmword import KWord

class Linker(AddressMapper):

    class Symbol(object):
        def __init__(self, address: KWord, size: int = 0, isWeak: bool = False):
            self.address = KWord(address, address.type)
            self.size = size
            self.isWeak = isWeak

        def __repr__(self) -> str:
            return f"repr={vars(self)}"

        def __str__(self) -> str:
            return f"Symbol container; {self.__repr__()}"

    class RelocFixup(object):
        def __init__(self, reloctype: ELFFlags.Reloc, source: KWord, dest: KWord):
            self.type = reloctype
            self.source = KWord(source, source.type)
            self.dest = KWord(dest, dest.type)

        def __repr__(self) -> str:
            return f"repr=({self.type}, {self.source:X}, {self.dest:X})"

        def __str__(self) -> str:
            return f"Relocation handler; {self.__repr__()}"

    def __init__(self, base: AddressMapper):
        super().__init__(base)
        self.baseAddress = KWord(0x80000000, KWord.Types.ABSOLUTE)
        self.ctorStart, self.ctorEnd = KWord(0, KWord.Types.ABSOLUTE), KWord(0, KWord.Types.ABSOLUTE)
        self.outputStart, self.outputEnd = KWord(0, KWord.Types.ABSOLUTE), KWord(0, KWord.Types.ABSOLUTE)
        self.bssStart, self.bssEnd = KWord(0, KWord.Types.ABSOLUTE), KWord(0, KWord.Types.ABSOLUTE)
        self.kamekStart, self.kamekEnd = KWord(0, KWord.Types.ABSOLUTE), KWord(0, KWord.Types.ABSOLUTE)

        # SECTIONS

        self._linked = False
        self._modules = {}
        self._binaries = []
        self._sectionBases = {}
        self._location = 0
        self._memory = BytesIO()

        # SYMBOLS

        self._globalSymbols = {}
        self._localSymbols = {}
        self._symbolTableContents = {}
        self._externSymbols = {}
        self._symbolSizes = {}

        # RELOCATIONS

        self._fixups = []

        # KAMEK HOOKS

        self._kamekRelocs = {}
        self._kamekHooks = []

    def __repr__(self) -> str:
        return f"repr={vars(self)}"

    def __str__(self) -> str:
        return f"Module linker; {self.__repr__()}"

    def __iadd__(self, file: str):
        with open(file, "rb") as _elf:
            self._modules[file] = ELFFile(BytesIO(_elf.read()))

    def __isub__(self, file: str):
        self._modules.pop(file, f"{file} does not exist in the current container")

    @property
    def bssSize(self) -> int:
        return self.bssEnd - self.bssStart

    # """ MODULES """

    def add_module(self, elf: str):
        if self._linked:
            raise AlreadyLinkedException("This linker has already been linked")
        if elf in self._modules.keys():
            raise AlreadyExistsException("This module is already part of this linker")

        self.__iadd__(elf)

    def clear_modules(self):
        self._modules = {}

    def remove_module(self, file: str):
        self.__isub__(file)

    def pop_module(self, file: str):
        return self._modules.pop(file)

    # """ LINKING """

    def link_static(self, symbolData: dict, baseAddr: int = None):
        if baseAddr:
            self.baseAddress = KWord(self.remap(baseAddr), KWord.Types.ABSOLUTE)

        self._do_link(symbolData)

    def link_dynamic(self, symbolData: dict):
        self.baseAddress = KWord(0, KWord.Types.RELATIVE)
        self._do_link(symbolData)

    def _do_link(self, symbolData: list):
        if self._linked:
            raise AlreadyLinkedException("This linker has already been linked")
        self._linked = True

        for key in symbolData:
            self._externSymbols[key] = self.remap(symbolData[key])

        self._collect_sections()
        self._build_symbol_tables()
        self._process_relocations()
        self._process_hooks()

    # """ SECTIONS """

    def _import_sections(self, prefix: str, alignEnd: int = 4):
        for elf in self._modules.values():
            for section in [section for section in elf.iter_sections() if section.name.startswith(prefix)]:
                self._sectionBases[section.name + str(section.data_size)] = KWord(self._location, KWord.Types.ABSOLUTE)
                
                if alignEnd > 0 and section.data_size % alignEnd != 0:
                    self._location += (section.data_size + (alignEnd-1)) & -alignEnd

                    padlen = alignEnd - (section.data_size % alignEnd)
                    self._binaries.append(BytesIO(section.data() + b"\x00" * padlen))
                else:
                    self._location += section.data_size
                    self._binaries.append(BytesIO(section.data()))

    def _collect_sections(self):
        self._location = KWord(self.baseAddress, KWord.Types.ABSOLUTE)
        self.outputStart = KWord(self._location, KWord.Types.ABSOLUTE)

        self._import_sections(".init")
        self._import_sections(".fini")
        self._import_sections(".text")

        self.ctorStart = KWord(self._location, KWord.Types.ABSOLUTE)
        self._import_sections(".ctors")
        self.ctorEnd = KWord(self._location, KWord.Types.ABSOLUTE)

        self._import_sections(".dtors")
        self._import_sections(".rodata")
        self._import_sections(".data", 32)

        self.bssStart = KWord(self.outputEnd, KWord.Types.ABSOLUTE)
        self._import_sections(".bss")
        self.bssEnd = KWord(self._location, KWord.Types.ABSOLUTE)

        self.kamekStart = KWord(self._location, KWord.Types.ABSOLUTE)
        self._import_sections(".kamek")
        self.kamekEnd = KWord(self._location, KWord.Types.ABSOLUTE)

        for binary in self._binaries:
            self._memory.write(binary.getvalue())

    # """ SYMBOLS """

    def _resolve_symbol(self, elfpath: str, name: str) -> Linker.Symbol:
        _locals = self._localSymbols[elfpath]
        if name in _locals:
            return _locals[name]
        elif name in self._globalSymbols:
            return self._globalSymbols[name]
        elif name in self._externSymbols:
            return Linker.Symbol(KWord(self._externSymbols[name], KWord.Types.ABSOLUTE))

        raise InvalidDataException(f"Undefined symbol {name}")

    def _build_symbol_tables(self):
        self._globalSymbols["__ctor_loc"] = Linker.Symbol(self.ctorStart)
        self._globalSymbols["__ctor_end"] = Linker.Symbol(self.ctorEnd)

        for _elfkey in self._modules:
            elf = self._modules[_elfkey]

            _locals = {}
            self._localSymbols[_elfkey] = _locals

            for s in [section for section in elf.iter_sections() if section.header["sh_type"] == "SHT_SYMTAB"]:
                strTabIdx = s.header["sh_link"]
                if strTabIdx <= 0 or strTabIdx >= elf.num_sections():
                    raise InvalidTableLinkageException("Symbol table is not linked to a string table")

                strTab = elf.get_section(int(strTabIdx))

                self._symbolTableContents[s.name + str(s.data_size)] = self._parse_symbol_table(elf, s, strTab, _locals)

    def _parse_symbol_table(self, elf: ELFFile, symTab: Section, strTab: Section, _locals: dict) -> list:
        if symTab.header["sh_entsize"] != 16:
            raise InvalidDataException("Invalid symbol table format (sh_entsize != 16)")
        if strTab.header["sh_type"] != "SHT_STRTAB":
            raise InvalidDataException("String table does not have type SHT_STRTAB")

        _symbolNames = [ None ]
        symbolData = BytesIO(symTab.data())
        symbolData.seek(16)

        count = int(symTab.data_size / 16)

        for _ in range(count - 1):
            st_name = read_uint32(symbolData)
            st_value = read_uint32(symbolData)
            st_size = read_uint32(symbolData)
            st_info = read_ubyte(symbolData)
            st_other = read_ubyte(symbolData)
            st_shndx = read_uint16(symbolData)

            _bind = st_info >> 4
            _type = st_info & 0xF

            name = read_string(BytesIO(strTab.data()), st_name)

            _symbolNames.append(name)

            if len(name) == 0 or st_shndx == 0:
                continue

            # What location is this referencing?
            if st_shndx < 0xFF00: # Reference
                refSection = elf.get_section(st_shndx)
            elif st_shndx == 0xFFF1: # Absolute symbol
                refSection = None
            else:
                raise InvalidDataException("Unknown section index found in symbol table")

            if st_shndx == 0xFFF1: # Absolute symbol
                addr = KWord(st_value, KWord.Types.ABSOLUTE)
            elif st_shndx < 0xFF00: # Reference
                _refkey = refSection.name + str(refSection.data_size)
                if _refkey not in self._sectionBases:
                    continue # Skip past unwanted symbols
                addr = KWord(self._sectionBases[refSection.name + str(refSection.data_size)] + st_value,
                             self._sectionBases[refSection.name + str(refSection.data_size)].type)
            else:
                raise NotImplementedError("Unknown section index found in symbol table")

            if _bind == ELFFlags.SymBind.STB_LOCAL:
                if name in _locals:
                    raise InvalidDataException(f"Redefinition of local symbol {name}")
                
                _locals[name] = Linker.Symbol(addr, st_size)
                self._symbolSizes[addr] = st_size

            elif _bind == ELFFlags.SymBind.STB_GLOBAL:
                if name in self._globalSymbols and not self._globalSymbols[name].isWeak:
                    raise InvalidDataException(f"Redefinition of global symbol {name}")

                self._globalSymbols[name] = Linker.Symbol(addr, st_size)
                self._symbolSizes[addr] = st_size
                
            elif _bind == ELFFlags.SymBind.STB_WEAK:
                if name not in self._globalSymbols:
                    self._globalSymbols[name] = Linker.Symbol(addr, st_size, isWeak=True)
                    self._symbolSizes[addr] = st_size

        return _symbolNames

    # """ RELOCATIONS """

    def _process_relocations(self):
        for _elfkey in self._modules:
            elf = self._modules[_elfkey]
            for s in [section for section in elf.iter_sections() if section.header["sh_type"] == "SHT_REL"]:
                raise InvalidDataException("OH CRAP ;P")

            for s in [section for section in elf.iter_sections() if section.header["sh_type"] == "SHT_RELA"]:
                if s.header["sh_info"] <= 0 or s.header["sh_info"] >= elf.num_sections():
                    raise InvalidDataException("Rela table is not linked to a section")
                if s.header["sh_link"] <= 0 or s.header["sh_link"] >= elf.num_sections():
                    raise InvalidDataException("Rela table is not linked to a symbol table")

                affected = elf.get_section(s.header["sh_info"])
                symTab = elf.get_section(s.header["sh_link"])
                
                self._process_rela_section(_elfkey, elf, s, affected, symTab)

    def _process_rela_section(self, elfpath: str, elf: ELFFile, relocs: Section, section: Section, symTab: Section):
        if relocs.header["sh_entsize"] != 12:
            raise InvalidDataException("Invalid relocs format (sh_entsize != 12)")
        if symTab.header["sh_type"] != "SHT_SYMTAB":
            raise InvalidDataException("Symbol table does not have type SHT_SYMTAB")

        relocData = BytesIO(relocs.data())
        count = int(relocs.data_size / 12)

        for _ in range(count):
            r_offset = read_uint32(relocData)
            r_info = read_uint32(relocData)
            r_addend = read_sint32(relocData)

            reloc = r_info & 0xFF
            symIndex = r_info >> 8
            _symkey = section.name + str(section.data_size)

            if symIndex == 0:
                raise InvalidDataException("Linking to undefined symbol")
            elif _symkey not in self._sectionBases:
                continue

            symName = self._symbolTableContents[symTab.name + str(symTab.data_size)][symIndex]

            source = KWord(self._sectionBases[section.name + str(section.data_size)] + r_offset, KWord.Types.ABSOLUTE)
            dest = KWord(self._resolve_symbol(elfpath, symName).address + r_addend, KWord.Types.ABSOLUTE)

            if not self._kamek_use_reloc(reloc, source, dest):
                self._fixups.append(Linker.RelocFixup(reloc, source, dest))

    # """ KAMEK HOOKS """

    def _kamek_use_reloc(self, _type: ELFFlags.Reloc, source: KWord, dest: KWord):
        if source < self.kamekStart or source >= self.kamekEnd:
            return False
        elif _type != ELFFlags.Reloc.R_PPC_ADDR32:
            raise InvalidOperationException("Unsupported relocation type in the Kamek hook data section")

        self._kamekRelocs[source] = dest
        return True

    def _process_hooks(self):
        for _elfkey in self._modules:
            for _symbolkey in self._localSymbols[_elfkey]:
                if _symbolkey.startswith(" kHook"):
                    cmdAddr = self._localSymbols[_symbolkey].address

                    self._memory.seek(cmdAddr - self.baseAddress)
                    argCount = read_uint32(self._memory)
                    _type = read_uint32(self._memory)
                    args = []

                    for i in range(argCount):
                        argAddr = cmdAddr + (8 + (i << 2))
                        if argAddr in self._kamekRelocs:
                            args.append(self._kamekRelocs[argAddr])
                        else:
                            self._memory.seek(argAddr - self.baseAddress)
                            args.append(KWord(read_uint32(self._memory), KWord.Types.VALUE))

                    self._kamekHooks.append(HookData(_type, args))

