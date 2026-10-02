from conan import ConanFile


class NmosCppConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps"
    # Conan Center recipes still pin older transitive deps (cpprestsdk → boost/1.83.0,
    # json-schema-validator → nlohmann_json/3.11.3). Force the versions we want for the
    # whole graph. Cap Boost at 1.86.0: 1.87 removed deprecated Asio APIs that unpatched
    # websocketpp 0.8.2 (and our own Asio usage) still need. without_cobalt avoids
    # packaging failures when Boost is built from source with OpenSSL present
    # (conan-center-index#30414).
    default_options = {
        "boost/*:shared": False,
        "boost/*:without_cobalt": True,
    }

    def requirements(self):
        self.requires("boost/1.86.0", force=True)
        self.requires("cpprestsdk/2.10.19")
        self.requires("websocketpp/0.8.2")
        self.requires("openssl/3.5.7")
        self.requires("json-schema-validator/2.4.0")
        self.requires("nlohmann_json/3.12.0", force=True)
        self.requires("zlib/1.3.2")
        self.requires("jwt-cpp/0.7.2")
