from __future__ import annotations

from io import BytesIO

from elfenums import ELFFlags
from exceptions import InvalidOperationException
from ioreader import write_uint32, write_uint16
from kmword import KWord

class Command(object):
    class KCmdID:
        Null = 0

        # these deliberately match the ELF relocations
        Addr32 = 1
        Addr16Lo = 4
        Addr16Hi = 5
        Addr16Ha = 6
        Rel24 = 10

        # these are new
        WritePointer = 1 # same as Addr32 on purpose
        Write32 = 32
        Write16 = 33
        Write8 = 34
        CondWritePointer = 35
        CondWrite32 = 36
        CondWrite16 = 37
        CondWrite8 = 38

        Branch = 64
        BranchLink = 65

    def __init__(self, kId: KCmdID, address: KWord):
        self.id = kId
        self.address = KWord(address, address.type)

class BranchCommand(Command):
    def __init__(self, source: KWord, target: KWord, isLink: bool):
        kId = Command.KCmdID.BranchLink if isLink else Command.KCmdID.Branch
        super().__init__(kId, source)
        self.target = KWord(target, target.type)

    def __repr__(self) -> str:
        return f"repr={vars(self)}"

    def __str__(self) -> str:
        return f"Branch Command; {self.__repr__()}"

    def write_arguments(self, io: BytesIO):
        self.target.assert_not_ambiguous()
        write_uint32(io, self.target)

    def is_equal_reloc_types(self) -> bool:
        return self.address.type == self.target.type

    def is_equal_reloc_absolute(self) -> bool:
        return self.is_equal_reloc_types() and self.target.type.is_absolute_address()

    def is_equal_reloc_relative(self) -> bool:
        return self.is_equal_reloc_types() and self.target.type.is_relative_address()

    def apply(self, file: "KamekBinary"):
        if self.is_equal_reloc_absolute() and file.contains(self.address):
            file.write_u32(self.address, self._generate_instruction())

    def pack_riivo(self) -> str:
        raise NotImplementedError()

    def pack_gecko_codes(self) -> list:
        raise NotImplementedError()

    def apply_to_dol(self):
        raise NotImplementedError()

    def _generate_instruction(self) -> int:
        delta = self.target - self.address
        insn = 0x48000001 if self.id == Command.KCmdID.BranchLink else 0x48000000
        return insn | (delta & 0x3FFFFFC)

class PatchExitCommand(Command):
    def __init__(self, source: KWord, target: KWord):
        super().__init__(Command.KCmdID.Branch, source)
        self.target = KWord(target, target.type)
        self.endAddress = KWord(0, KWord.Types.ABSOLUTE)

    def __repr__(self) -> str:
        return f"repr={vars(self)}"

    def __str__(self) -> str:
        return f"Exit Patch Command; {self.__repr__()}"

    def write_arguments(self, io: BytesIO):
        self.endAddress.assert_not_ambiguous()
        self.target.assert_not_ambiguous()
        write_uint32(io, self.endAddress)
        write_uint32(io, self.target)

    def is_equal_reloc_types(self) -> bool:
        return self.address.type == self.target.type == self.endAddress.type

    def is_equal_reloc_absolute(self) -> bool:
        return self.is_equal_reloc_types() and self.target.type.is_absolute_address()

    def is_equal_reloc_relative(self) -> bool:
        return self.is_equal_reloc_types() and self.target.type.is_relative_address()

    def apply(self, file: "KamekBinary") -> bool:
        funcSize = file.get_symbol_size(self.address)
        funcEnd = self.address + (funcSize - 4)

        if funcSize < 4:
            raise InvalidOperationException("Queried function is too small")

        if file.read_u32(funcEnd) != 0x4E800020:
            raise InvalidOperationException("Function does not end in blr")

        instrLoc = self.address
        while instrLoc < funcEnd:
            insn = file.read_u32(instrLoc)
            if (insn & 0xFC00FFFF == 0x4C000020):
                raise InvalidOperationException("Function contains a return partway through")

        self.endAddress = funcEnd
        if self.is_equal_reloc_absolute() and file.contains(self.address):
            file.write_u32(self.endAddress, self._generate_instruction())
            return True
        else:
            return False

    def pack_riivo(self) -> str:
        raise NotImplementedError()

    def apply_to_dol(self):
        raise NotImplementedError()

    def _generate_instruction(self) -> int:
        delta = self.target - self.address
        insn = 0x48000001 if self.id == Command.KCmdID.BranchLink else 0x48000000
        return insn | (delta & 0x3FFFFFC)
     
class WriteCommand(Command):
    class Type:
        Pointer = 1
        Value32 = 2
        Value16 = 3
        Value8 = 4

    def __init__(self, address: KWord, value: KWord, valueType: Type, original: KWord = None):
        super().__init__(self.id_from_type(valueType, original != None), address)
        self.value = KWord(value, value.type)
        self.valueType = valueType

        if original:
            self.original = KWord(original, original.type)
        else:
            self.original = None

    def __repr__(self) -> str:
        return f"repr={vars(self)}"
    
    def __str__(self) -> str:
        return f"Write Command; {self.__repr__()}"

    @staticmethod
    def id_from_type(_type: Type, isConditional: bool) -> Type:
        if isConditional:
            if _type == WriteCommand.Type.Pointer:
                return Command.KCmdID.CondWritePointer
            elif _type == WriteCommand.Type.Value32:
                return Command.KCmdID.CondWrite32
            elif _type == WriteCommand.Type.Value16:
                return Command.KCmdID.CondWrite16
            elif _type == WriteCommand.Type.Value8:
                return Command.KCmdID.CondWrite8
        else:
            if _type == WriteCommand.Type.Pointer:
                return Command.KCmdID.WritePointer
            elif _type == WriteCommand.Type.Value32:
                return Command.KCmdID.Write32
            elif _type == WriteCommand.Type.Value16:
                return Command.KCmdID.Write16
            elif _type == WriteCommand.Type.Value8:
                return Command.KCmdID.Write8

        raise NotImplementedError(f"Unimplemented command type {_type} specified")

    def write_arguments(self, io: BytesIO):
        if self.valueType == WriteCommand.Type.Pointer:
            self.value.assert_not_ambiguous()
        else:
            self.value.assert_value()

        write_uint32(io, self.value)
        
        if self.original is not None:
            self.original.assert_not_relative()
            write_uint32(io, self.original)

    def pack_riivo(self) -> str:
        raise NotImplementedError()

    def pack_gecko_codes(self) -> list:
        raise NotImplementedError()

    def apply_to_dol(self):
        raise NotImplementedError()

class RelocCommand(Command):
    def __init__(self, source: KWord, target: KWord, reloc: ELFFlags.Reloc):
        super().__init__(reloc, source)
        self.target = KWord(target, target.type)

    def __repr__(self) -> str:
        return f"repr={vars(self)}"
    
    def __str__(self) -> str:
        return f"Relocation Command; {self.__repr__()}"

    def is_equal_reloc_types(self) -> bool:
        return self.address.type == self.target.type

    def is_equal_reloc_absolute(self) -> bool:
        return self.is_equal_reloc_types() and self.target.type.is_absolute_address()

    def is_equal_reloc_relative(self) -> bool:
        return self.is_equal_reloc_types() and self.target.type.is_relative_address()

    def write_arguments(self, io: BytesIO):
        self.target.assert_not_ambiguous()
        write_uint32(io, self.target)

    def apply(self, file: "KamekBinary") -> bool:
        if self.id == Command.KCmdID.Rel24:
            if self.is_equal_reloc_types() and not self.target.is_value():
                delta = self.target - self.address

                insn = (file.read_u32(self.address) & 0xFC000003) | (delta & 0x3FFFFFC)
                file.write_u32(self.address, insn)
                return True
 
        elif self.id == Command.KCmdID.Addr32:
            if self.target.is_absolute_addr():
                file.write_u32(self.address, self.target & 0xFFFFFFFF)
                return True

        elif self.id == Command.KCmdID.Addr16Lo:
            if self.target.is_absolute_addr():
                file.write_u16(self.address, self.target & 0xFFFF)
                return True

        elif self.id == Command.KCmdID.Addr16Hi:
            if self.target.is_absolute_addr():
                file.write_u16(self.address, (self.target >> 16) & 0xFFFF)
                return True

        elif self.id == Command.KCmdID.Addr16Ha:
            if self.target.is_absolute_addr():
                aTarget = (self.target >> 16) & 0xFFFF if self.target & 0x8000 != 0 else ((self.target >> 16) + 1) & 0xFFFF
                file.write_u16(self.address, aTarget)
                return True

        else:
            raise NotImplementedError("Unrecognized relocation type")

        return False
        

    def pack_riivo(self) -> str:
        raise NotImplementedError()

    def apply_to_dol(self):
        raise NotImplementedError()