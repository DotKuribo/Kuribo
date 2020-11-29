from __future__ import annotations

from exceptions import InvalidOperationException

class KWord(int):
    class Types:
        VALUE = 1
        ABSOLUTE = 2
        RELATIVE = 3

    def __init__(self, value: int = 0, _type: KWord.Types = Types.VALUE):
        self.type = _type

    def __new__(cls, value, *args, **kwargs):
        return super(KWord, cls).__new__(cls, value)

    def __repr__(self) -> str:
        return f"repr=(Value: {self.real}, {vars(self)})"

    def __str__(self) -> str:
        return f"Kamek extended integer; {self.__repr__()}"

    def is_absolute_addr(self) -> bool:
        return self.type == KWord.Types.ABSOLUTE

    def is_relative_addr(self) -> bool:
        return self.type == KWord.Types.RELATIVE

    def is_value(self) -> bool:
        return self.type == KWord.Types.VALUE

    def assert_value(self):
        if not self.is_value():
            raise InvalidOperationException(f"KWord {self.real} must be a value in this context")

    def assert_not_relative(self):
        if not self.is_relative_addr():
            raise InvalidOperationException(f"KWord {self.real} must not be a relative address in this context")

    def assert_not_ambiguous(self):
        if self.is_absolute_addr() and (self & 0x80000000) == 0:
            raise InvalidOperationException("Address is ambiguous: absolute, top bit not set")
        if self.is_relative_addr() and (self & 0x80000000) != 0:
            raise InvalidOperationException("Address is ambiguous: relative, top bit set")
