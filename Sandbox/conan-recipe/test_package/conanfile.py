import os

from conans import ConanFile, CMake, tools


class NmosCppTestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        # hmm, why does CMake report e.g. "Library nmos-cpp not found in package, might be system one"?
        # yet it still works...
        cmake.build()

    def test(self):
        if not tools.cross_building(self):
            os.chdir("bin")
            self.run(".%snmos-cpp-package-test" % os.sep)
