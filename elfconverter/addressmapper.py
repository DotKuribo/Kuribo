from __future__ import annotations

from exceptions import InvalidMappingException

class AddressMapper(object):

    class Mapping(object):
        def __init__(self, start: int, end: int, delta: int):
            self.start = start
            self.end = end
            self.delta = delta

        def __repr__(self) -> str:
            sign = "+" if self.delta >= 0 else "-"
            return f"{self.start:8X}-{self.end:8X}: {sign}0x{abs(self.delta)}"

        def __str__(self) -> str:
            return self.__repr__()

        def overlaps(self, other: AddressMapper.Mapping) -> bool:
            return (self.end >= other.start) and (self.start <= other.end)

    def __init__(self, base: AddressMapper = None):
        self.base = base
        self._mappings = []

    def add_mapping(self, start: int, end: int, delta: int):
        if start > end:
            raise InvalidMappingException(f"Cannot map {start:8X}-{end:8X} as start is higher than end")

        newMapping = AddressMapper.Mapping(start, end, delta)

        for mapping in self._mappings:
            if mapping.overlaps(newMapping):
                raise InvalidMappingException(f"New mapping {newMapping} overlaps existing mapping {mapping}")

        self._mappings.append(newMapping)

    def remap(self, addr: int) -> int:
        if self.base is not None:
            addr = self.base.remap(addr)

        for mapping in self._mappings:
            if (addr >= mapping.start) and (addr <= mapping.end):
                return addr + mapping.delta

        return addr