from conan import ConanFile

class NmosCppConan(ConanFile):
    name = "nmos-cpp"
    version = "4.12.0"
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "Development/*"
    generators = "CMakeDeps", "CMakeToolchain"
    requires = [
        "boost/1.83.0",
        "cpprestsdk/2.10.18",
        "websocketpp/0.8.2",
        "openssl/3.1.3",
        "json-schema-validator/2.1.0",
        "spdlog/1.12.0"
    ]

    def layout(self):
        self.folders.source = "Development"
