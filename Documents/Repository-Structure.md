# Repository Structure

- [Development](../Development)  
    C++ source code and build files for the software
    - [bst](../Development/bst)  
      Facades and adaptors to handle different C++ Standard Library implementations and Testing Frameworks
    - [cmake](../Development/cmake)  
      CMake instructions for making all the nmos-cpp libraries and test suite, used by the top-level **[CMakeLists.txt](../Development/CMakeLists.txt)**
    - [cpprest](../Development/cpprest)  
      Extensions to the [C++ REST SDK](https://github.com/Microsoft/cpprestsdk)
    - [detail](../Development/detail)  
      Small general purpose utilties and header files to facilitate cross-platform development
    - [mdns](../Development/mdns)  
      A simple API for mDNS Service Discovery (DNS-SD) and an implementation using the original Bonjour *dns_sd.h* API
    - [nmos](../Development/nmos)  
      Implementations of the **NMOS Node, Registration and Query APIs, and the NMOS Connection API** including SDP creation/processing for ST 2110 streams
    - [nmos-cpp-node](../Development/nmos-cpp-node)  
      An example **NMOS Node**, utilising the nmos module
    - [nmos-cpp-registry](../Development/nmos-cpp-registry)  
      A simple but functional instance of an **NMOS Registration & Discovery System (RDS)**, utilising the nmos module
    - [nmos-cpp-test](../Development/nmos-cpp-test)  
      The test suite runner, incorporating the tests for all modules
    - [pplx](../Development/pplx)  
      Extensions to the Parallel Patterns Library for task-based concurrent programming (part of the C++ REST SDK)
    - [rql](../Development/rql)  
      An implementation of the [Resource Query Language](https://github.com/persvr/rql)
    - [sdp](../Development/sdp)  
      An extensible implementation of a [Session Description Protocol](https://tools.ietf.org/html/rfc4566) parser/generator (to/from a JSON representation)
    - [slog](../Development/slog)  
      The slog Logging Library single header version
    - [third_party](../Development/third_party)  
      Third-party source files used by the nmos module
      - [catch](../Development/third_party/catch)  
        The [Catch](https://github.com/philsquared/Catch) (automated test framework) single header version
      - [cmake](../Development/third_party/cmake)  
        CMake modules derived from third-party sources
      - [mDNSResponder](../Development/third_party/mDNSResponder)  
        Patches and patched source files for the Bonjour DNS-SD implementation
      - [nlohmann](../Development/third_party/nlohmann)  
        The [JSON for Modern C++](https://github.com/nlohmann/json) and [Modern C++ JSON schema validator](https://github.com/pboettch/json-schema-validator) libraries
      - [nmos-device-connection-management](../Development/third_party/nmos-device-connection-management)  
        The JSON Schema files used for validation of Connection API requests
      - [nmos-discovery-registration](../Development/third_party/nmos-discovery-registration)  
        The JSON Schema files used for validation of e.g. Registration API requests
- [Documents](../Documents)  
  Documentation
