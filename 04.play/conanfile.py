from conan import ConanFile
from conan.tools.cmake.layout import cmake_layout;

class PlayRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    options = {"shared" : [True, False]}
    default_options = {"shared" : True}

    def requirements(self):
        self.requires("fmt/[~11]", override=True)
        self.requires("boost/[]", override=True)
        self.requires("folly/[]")
        self.requires("gtest/[^1]")

    def layout(self):
        cmake_layout(self)
