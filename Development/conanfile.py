from conan import ConanFile


class NmosCppConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps"
    # The Conan Center recipes for cpprestsdk and websocketpp pin boost/1.83.0, so force
    # the Boost version for the whole graph. Cap Boost at 1.86.0: 1.87 removed deprecated
    # Asio APIs that cpprestsdk 2.10.19 and unpatched websocketpp 0.8.2 still need.
    default_options = {
        "boost/*:shared": False,
    }

    def requirements(self):
        self.requires("boost/1.86.0", force=True)
        self.requires("cpprestsdk/2.10.19")
        self.requires("websocketpp/0.8.2")
        self.requires("openssl/3.5.7")
        self.requires("json-schema-validator/2.4.0")
        self.requires("nlohmann_json/3.12.0")
        self.requires("zlib/1.3.2")
        self.requires("jwt-cpp/0.7.2")
