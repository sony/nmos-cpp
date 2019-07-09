# Source and Build Files

C++ source code and build files for the software

- [boost](boost)  
  Extensions to the [Boost C++ Libraries](http://www.boost.org/)
- [bst](bst)  
  Facades and adaptors to handle different C++ Standard Library implementations and Testing Frameworks
- [cmake](cmake)  
  CMake instructions for making all the nmos-cpp libraries and test suite, used by the top-level **[CMakeLists.txt](CMakeLists.txt)**
- [cpprest](cpprest)  
  Extensions to the [C++ REST SDK](https://github.com/Microsoft/cpprestsdk)
- [detail](detail)  
  Small general purpose utilties and header files to facilitate cross-platform development
- [mdns](mdns)  
  A simple API for mDNS Service Discovery (DNS-SD) and an implementation using the original Bonjour *dns_sd.h* API
- [nmos](nmos)  
  Implementations of the **NMOS Node, Registration and Query APIs, and the NMOS Connection API** including SDP creation/processing for ST 2110 streams
- [nmos-cpp-node](nmos-cpp-node)  
  An example **NMOS Node**, utilising the nmos module
- [nmos-cpp-registry](nmos-cpp-registry)  
  A simple but functional instance of an **NMOS Registration & Discovery System (RDS)**, utilising the nmos module
- [nmos-cpp-test](nmos-cpp-test)  
  The test suite runner, incorporating the tests for all modules
- [pplx](pplx)  
  Extensions to the Parallel Patterns Library for task-based concurrent programming (part of the C++ REST SDK)
- [rql](rql)  
  An implementation of the [Resource Query Language](https://github.com/persvr/rql)
- [sdp](sdp)  
  An extensible implementation of a [Session Description Protocol](https://tools.ietf.org/html/rfc4566) parser/generator (to/from a JSON representation)
- [slog](slog)  
  The slog Logging Library single header version
- [third_party](third_party)  
  Third-party source files used by the nmos-cpp libraries
