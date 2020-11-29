from __future__ import annotations

from addressmapper import AddressMapper
from exceptions import InvalidDataException
from kmcommands import BranchCommand, WriteCommand, PatchExitCommand
from kmword import KWord

class HookData(object):
    def __init__(self, _type: int, args: list):
        self.type = _type
        self.args = args

    def __repr__(self) -> str:
        return f"repr={vars(self)}"

    def __str__(self) -> str:
        return f"Kamek HookData packet; {self.__repr__()}"

class KHook(object):
    def __init__(self):
        self.commands = []

    @staticmethod
    def create(data: HookData, mapper: AddressMapper) -> KHook:
        if data.type == 1:
            return WriteHook(data.args, mapper, False)
        elif data.type == 2:
            return WriteHook(data.args, mapper, True)
        elif data.type == 3:
            return BranchHook(data.args, mapper, False)
        elif data.type == 4:
            return BranchHook(data.args, mapper, True)
        elif data.type == 5:
            return PatchExitHook(data.args, mapper)
        else:
            raise NotImplementedError(f"Unknown command type: {data.type}")

    @staticmethod
    def get_value_arg(word: KWord) -> KWord:
        word.assert_value()
        return word

    @staticmethod
    def get_absolute_arg(word: KWord, mapper: AddressMapper) -> KWord:
        if word.type != KWord.Types.ABSOLUTE:
            word.assert_value()
            return KWord(mapper.remap(word), KWord.Types.ABSOLUTE)

        return word

    @staticmethod
    def get_any_pointer_arg(word: KWord, mapper: AddressMapper) -> KWord:
        if word.type == KWord.Types.VALUE:
            return KWord(mapper.remap(word), KWord.Types.ABSOLUTE)
        elif word.type in (KWord.Types.ABSOLUTE, KWord.Types.RELATIVE):
            return word
        else:
            raise NotImplementedError()

class BranchHook(KHook):
    def __init__(self, args: list, mapper: AddressMapper, isLink: bool):
        super().__init__()

        if len(args) != 2:
            raise InvalidDataException("Wrong arg count for BranchCommand")

        source = self.get_absolute_arg(args[0], mapper)
        dest = self.get_any_pointer_arg(args[1], mapper)

        self.commands.append(BranchCommand(source, dest, isLink))

class PatchExitHook(KHook):
    def __init__(self, args: list, mapper: AddressMapper):
        super().__init__()

        if len(args) != 2:
            raise InvalidDataException("Wrong arg count for PatchExitCommand")

        function = args[0]
        dest = self.get_any_pointer_arg(args[1], mapper)

        if not args[1].is_value() or args[1] != 0:
            self.commands.append(PatchExitCommand(function, dest))

class WriteHook(KHook):
    def __init__(self, args: list, mapper: AddressMapper, isConditional: bool):
        argcount = 4 if isConditional else 3
        if len(args) != argcount:
            raise InvalidDataException("Wrong arg count for WriteCommand")

        _type = self.get_value_arg(args[0])
        address = self.get_absolute_arg(args[1], mapper)
        original = None

        if _type == WriteCommand.Type.Pointer:
            value = self.get_any_pointer_arg(args[2], mapper)
            if isConditional:
                original = self.get_any_pointer_arg(args[3], mapper)
        else:
            value = self.get_value_arg(args[2])
            if isConditional:
                original = self.get_value_arg(args[3])

        self.commands.append(WriteCommand(address, value, _type, original))
