import os, subprocess, shutil, glob, json
from pathlib import Path
import sys


def system_cmd(cmd):
    # print(cmd)
    if subprocess.call(cmd, shell=True):
        print(cmd)
        raise SystemExit("Fail")


def require_dir(path):
    # TODO: if not isdir, delete
    if not os.path.exists(path):
        os.makedirs(path)


# Represents a toolkit -- one dir
class SimpleUtil:
    def __init__(self, path):
        self.path = path

    def run_with_args(self, tool, args):
        return system_cmd(os.path.join(self.path, tool) + " " + args)


class Aria(SimpleUtil):
    def __init__(self, path):
        super().__init__(path)

    def convert_cmx(self, elf, dst):
        return self.run_with_args("aria.exe", "-t CMX  -i %s -o %s" % (elf, dst))


class Clang(SimpleUtil):
    def __init__(self, path, game):
        super().__init__(path)

        self.game = game

        self.options = {
            "target": "ppc-linux-eabi",
            "exceptions": False,
            "rtti": False,
            "data-sections": True,
            "function-sections": True,
            "lto": True,
            "std": "c++17"
        }
        self.warnings = {
            "deprecated": False,
            "multichar": False
        }
        self.system_defines = [
            "EA_PLATFORM_LINUX",
            "EA_COMPILER_CPP17_ENABLED",
            "EA_COMPILER_CPP14_ENABLED",
            "EA_CPP14_CONSTEXPR=constexpr",
            "KURIBO_PLATFORM_WII=1",
            "__powerpc__",
            "CHAISCRIPT_NO_THREADS"
        ]
        self.system_includes = [
            "./source",
            "./source/vendor",
            "./",
            "./vendor"
        ]

        # LINKER
        self.linker_arg = "-T ..\\arch\\gc.lcf --gc-sections --Map ../bin/out.map"

    def get_option(self, key):
        return self.options[key]

    def set_option(self, key, value):
        self.options[key] = value

    # TODO: Support overriding system defs
    def coagulate_arguments(self, defs, includes):
        args = ["-c"]
        for k, v in self.options.items():
            if type(v) == bool:
                # Flag
                args += ["%s%s" % ("-f" if v else "-fno-", k)]
            elif type(v) == str:
                args += ["--%s=%s" % (k, v)]
        for k, v in self.warnings.items():
            args += ["%s%s" % ("-W" if v else "-Wno-", k)]

        return args + ["-D%s" % s for s in self.system_defines + defs] + ["-I%s" % s for s in
                                                                          self.system_includes + includes]

    def compile(self, src, dst, defs, includes, additional):
        args = self.coagulate_arguments(defs, includes)
        args += [src, "-o", dst] + additional
        argx = " ".join(args)
        if src.endswith(".c"):
            argx = argx.replace("--std=c++17", "")
        self.run_with_args("clang++.exe" if not src.endswith(".c") else "clang.exe", argx)

    def link(self, objects, dst):
        self.run_with_args("ld.lld.exe", self.linker_arg + " " + " ".join(objects) + " -o " + dst)

    def archive(self, objects, dst):
        self.run_with_args("llvm-ar.exe", "rcs %s " % dst + " ".join(objects))


class System:
    def __init__(self, clang_path, aria_path, game):
        self.clang = Clang(clang_path, game)
        self.aria = Aria(aria_path)


class Project:
    def __init__(self, cfg_path):
        self.cfg = {}
        with open(cfg_path, 'r') as cfg:
            self.cfg = json.load(cfg)
        if "module_name" not in self.cfg or "type" not in self.cfg:
            raise SystemExit("Cannot proceed: Module name must be defined")

    def build(self, source_dir, intermediate_dir, output_dir, system, debug=False):
        # Compile objects
        get_sources = lambda src, filter: [str(x) for x in Path(src).glob(filter)]

        cpp = get_sources(source_dir, "**/*.cxx")
        c = get_sources(source_dir, "**/*.c")
        objects = []

        for src in [cpp + c][0]:
            out = "%s/%s" % (intermediate_dir, src.replace(".cxx", ".o").replace(".c", ".o").replace("\\", "_"))
            objects.append(out)
            system.clang.compile(src, out,
                                 [
                                     # TODO - Region subversion
                                     "KURIBO_GAME_REGION_PAL",
                                     "KURIBO_DEBUG" if debug else "KURIBO_RELEASE"],
                                 [

                                     "C:\\devkitPro\\devkitPPC\\powerpc-eabi\\include\\c++\\6.3.0",
                                     "C:\\devkitPro\\devkitPPC\\powerpc-eabi\\include",
                                     "C:\\devkitPro\\devkitPPC\\powerpc-eabi\\include\\c++\\6.3.0\\powerpc-eabi",

                                     r"..\source",
                                     r"..\source\vendor"
                                  ],
                                 ["-O3" if not debug else "-O0", "-mcpu=750cl"]  # ["-Os"]
                                 )

        # Synthesize
        output_path = os.path.join(output_dir, self.cfg["module_name"])
        if debug: output_path += "D"

        if self.cfg["type"] == "static_library":
            system.clang.archive(objects, output_path + ".a")
            return output_path + '.a'
        elif self.cfg["type"] == "comet":
            if "libraries" in self.cfg:
                for lib in self.cfg["libraries"]:
                    lib = os.path.join(os.path.join(output_dir, "../"), lib) + "/" + lib
                    if debug: lib += "D"
                    lib += ".a"
                    objects += [lib]
            system.clang.link(objects, output_path + ".elf")
            # system.aria.convert_cmx(output_path + ".elf", output_path + ".cmx")
            system_cmd("C:\\devkitPro\\devkitPPC\\bin\\powerpc-eabi-objcopy.exe -O binary " + output_path + ".elf " + output_path + ".cmx")
            return output_path + ".cmx"


class ProjectBuilder:
    def __init__(self, json_path, path, bin, binint, system):
        self.project = Project(json_path)
        self.path = path
        require_dir(bin)
        self.bin = os.path.join(bin, "<VER>-revolution-ppc32")
        self.bin = os.path.join(self.bin, self.project.cfg["module_name"])

        self.binint = os.path.join(binint, "<VER>-revolution-ppc32")
        self.binint = os.path.join(self.binint, self.project.cfg["module_name"])

        self.system = system

    def build(self, debug=False):
        binint = self.binint.replace("<VER>", "Debug" if debug else "Release")
        bin = self.bin.replace("<VER>", "Debug" if debug else "Release")
        require_dir(Path(bin).parent)
        require_dir(Path(binint).parent)
        require_dir(bin)
        require_dir(binint)
        return self.project.build(self.path,
                                  binint,
                                  bin,
                                  self.system, debug)


if __name__ == "__main__":
    debug = True
    if len(sys.argv) > 1: debug = {"release": False, "debug": True}[sys.argv[1]]
    game = 'MKW'

    system = System("", os.path.join(os.path.dirname(os.path.realpath(__file__)), "tool"), game)


    def build_module(name):
        print("===\nBuilding Module: %s" % name)
        os.chdir("source")
        ncore = ProjectBuilder("./project.json", "./", "../bin/", "../bin-int/", system).build(debug)
        os.chdir("../")
        print("===\n")
        return ncore


    build_module("kuribo")
