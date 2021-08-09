import os

from conans import ConanFile, CMake, tools


class NmosCppTestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    # use cmake_find_package_multi because the project installs a config-file package
    generators = "cmake", "cmake_find_package_multi"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        # hmm, on Windows why does CMake report e.g. "Library nmos-cpp not found in package, might be system one"?
        # yet it still works...
        cmake.build()

    def test(self):
        if not tools.cross_building(self):
            bin_path = os.path.join("bin", "test_package")
            self.run(bin_path, run_environment=True)
