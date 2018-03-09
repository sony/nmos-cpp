# Repository Structure

- [Development](../Development)  
    C++ source code and build files for the software
    - [bst](../Development/bst)  
      Facades and adaptors to handle different C++ Standard Library implementations and Testing Frameworks
    - [cpprest](../Development/cpprest)  
      Extensions to the [C++ REST SDK](https://github.com/Microsoft/cpprestsdk)
    - [detail](../Development/detail)  
      Small general purpose utilties and header files to facilitate cross-platform development
    - [mdns](../Development/mdns)  
      A simple API for mDNS Service Discovery (DNS-SD) and an implementation using the original Bonjour *dns_sd.h* API
    - [nmos](../Development/nmos)  
      Implementations of the **NMOS Node, Registration and Query APIs, and the NMOS Connection API**
    - [nmos-cpp-node](../Development/nmos-cpp-node)  
      An example **NMOS Node**, utilising the nmos module
    - [nmos-cpp-registry](../Development/nmos-cpp-registry)  
      A simple but functional instance of an **NMOS Registration & Discovery System (RDS)**, utilising the nmos module
    - [rql](../Development/rql)  
      An implementation of the [Resource Query Language](https://github.com/persvr/rql)
    - [slog](../Development/slog)  
      The slog Logging Library single header version
    - [third_party](../Development/third_party)
      - [catch](../Development/third_party/catch)  
        The [Catch](https://github.com/philsquared/Catch) (automated test framework) single header version
      - [cmake](../Development/third_party/cmake)  
        CMake modules derived from third-party sources
      - [mDNSResponder](../Development/third_party/mDNSResponder)  
        Patches for the Bonjour DNS-SD implementation
- [Documents](../Documents)  
  Documentation
