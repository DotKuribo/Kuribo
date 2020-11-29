from __future__ import annotations

import argparse
import os
import sys
import re
import shutil
import tempfile

from elftools.elf.elffile import ELFFile

from addressmapper import AddressMapper
from exceptions import InvalidDataException
from linker import Linker
from kamek import KamekBinary
from versionmap import VersionMapper

class ElfHandler(Linker):
    def __init__(self, base: AddressMapper, files: str):
        super().__init__(base)

        self.outputPath = None
        self.versionMap = None
        self.externals = {}

        if isinstance(files, str):
            self.add_module(files)
        else:
            for file in files:
                self.add_module(file)

    def __repr__(self):
        return f"repr={vars(self)}"

    def __str__(self):
        return f"ELF module converter; {self.__repr__()}"

    @staticmethod
    def read_externals(file: str) -> dict:
        symbolDict = {}
        assignmentRegex = re.compile(r"^\s*([a-zA-Z0-9_<>\$]+)\s*=\s*0x([a-fA-F0-9]+)\s*(#.*)?$")

        with open(file, "r") as f:
            for i, line in enumerate(f.readlines()):
                if line.strip() == "" or line.strip().startswith("#") or line.strip().startswith("//"):
                    continue

                try:
                    match = re.findall(assignmentRegex, line.strip())
                    _symbol = match[0][0]
                    _address = match[0][1]
                except IndexError:
                    raise InvalidDataException(f"Symbol definition {_symbol} at line {i} is an invalid entry")

                try:
                    symbolDict[_symbol] = int(_address, 16)
                except ValueError:
                    raise InvalidDataException(f"Address {_address} at line {i} is not a hexadecimal number")

        return symbolDict

    def exec_jobs(self):
        pass

def main():
    parser = argparse.ArgumentParser("elftokuribo", description="ELF to Kuribo module converter")

    parser.add_argument("elf", help="ELF object file(s)", nargs="+")
    parser.add_argument("--dynamic", help="The module is dynamically relocated", action="store_true")
    parser.add_argument("--static", help="The module is statically located at ADDR", metavar="ADDR")
    parser.add_argument("--externals", help="External linker map", metavar="FILE")
    parser.add_argument("--versionmap", help="Version map for address translations", metavar="FILE")
    parser.add_argument("-d", "--dest", help="Destination path", metavar="FILE")

    args = parser.parse_args()

    if args.dynamic and args.static:
        parser.error("Args `--dynamic' and `--static' cannot be used together")
    elif not args.dynamic and not args.static:
        parser.error("Must provide either `--dynamic' or `--static' arguments")

    _externals = None
    _versionMap = None

    if args.dynamic:
        _baseAddr = None
    elif args.static:
        _baseAddr = int(args.static, 16)

    if args.externals:
        _externals = ElfHandler.read_externals(os.path.abspath(os.path.normpath(args.externals)))

    if args.versionmap:
        _versionMap = VersionMapper(os.path.abspath(os.path.normpath(args.versionMap)))
    else:
        _versionMap = VersionMapper()

    if args.dest:
        _dest = os.path.abspath(os.path.normpath(args.dest))
    else:
        _dest = os.path.abspath("build-$KV$.kmk")

    print(_versionMap)

    for versionKey in _versionMap.mappers:
        print(f"Linking version {versionKey}")

        elfConverter = ElfHandler(_versionMap.mappers[versionKey], args.elf)

        if _baseAddr:
            elfConverter.link_static(_externals, _baseAddr)
        else:
            elfConverter.link_dynamic(_externals)

        kf = KamekBinary()
        kf.load_from_linker(elfConverter)
        with open(_dest.replace("$KV$", versionKey), "wb") as kBinary:
            kBinary.write(KamekBinary.pack_from(elfConverter).getvalue())

    print("Finished execution")

if __name__ == "__main__":
    main()
    
    


    


