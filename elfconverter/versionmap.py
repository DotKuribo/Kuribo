import os
import re

from exceptions import InvalidDataException
from addressmapper import AddressMapper

class VersionMapper(object):
    def __init__(self, path: str = None):
        self.mappers = {}
        if path is not None:
            self.open_version_map(path)
        else:
            self.mappers["default"] = AddressMapper()

    def __repr__(self) -> str:
        return f"repr={vars(self)}"
    
    def __str__(self) -> str:
        return f"Version Mapper; {self.__repr__()}"

    def open_version_map(self, path):
        commentRegex = re.compile(r"^\s*#")
        emptyLineRegex = re.compile(r"^\s*$")
        sectionRegex = re.compile(r"^\s*\[([a-zA-Z0-9_.]+)\]$")
        extendRegex = re.compile(r"^\s*extend ([a-zA-Z0-9_.]+)\s*(#.*)?$")
        mappingRegex = re.compile(r"^\s*([a-fA-F0-9]{8})-((?:[a-fA-F0-9]{8})|\*)\s*:\s*([-+])0x([a-fA-F0-9]+)\s*(#.*)?$")

        curVersionName = None
        curVersion = None

        with open(path, "r") as vMap:
            for i, line in enumerate(vMap.readlines()):
                if line.strip() == "":
                    continue
                elif re.match(commentRegex, line):
                    continue

                if match := re.match(sectionRegex, line):
                    curVersionName = match.groups()[0]
                    if curVersionName in self.mappers:
                        InvalidDataException(f"Versions file contains duplicate version name {curVersionName}")
                    
                    curVersion = AddressMapper()
                    self.mappers[curVersionName] = curVersion
                elif curVersion is not None:
                    if match := re.match(extendRegex, line):
                        baseName = match.groups()[0]
                        if baseName not in self.mappers:
                            raise InvalidDataException(f"Version {curVersionName} extends unknown version {baseName}")
                        elif curVersion.base is not None:
                            InvalidDataException(f"version {curVersionName} already extends a version")
                        
                        curVersion.base = self.mappers[baseName]
                    elif match := re.match(mappingRegex, line):
                        startAddress = int(match.groups()[0], 16)
                        if match.groups()[1] == "*":
                            endAddress = 0xFFFFFFFF
                        else:
                            endAddress = int(match.groups()[1], 16)
                        
                        delta = int(match.groups()[3], 16)
                        if match.groups()[2] == "-":
                            delta = -delta

                        curVersion.add_mapping(startAddress, endAddress, delta)
                    else:
                        print(f"Bad data at line {i} in versions file")

                        
                
verMapper = VersionMapper(r"C:\Users\Kyler-Josh\source\repos\Kamek\examples\versions-nsmbw.txt")