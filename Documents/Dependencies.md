# Dependencies

This software is being developed for multiple platforms (Windows, Mac OS X, Linux) using portable C++.

The codebase utilizes a number of great open-source projects (licenses vary).

- The [CMake](https://cmake.org/) build tools.
- The [C++ REST SDK](https://github.com/Microsoft/cpprestsdk), for client-server communication over REST APIs
  - This library incorporates some third-party material including WebSocket++, and also relies on e.g. some of the Boost C++ Libraries and OpenSSL.
- The [Bonjour SDK](https://developer.apple.com/bonjour/), to implement DNS Service Discovery (DNS-SD)
- The [WebSocket++](https://github.com/zaphoyd/websocketpp) header-only C++ websocket client/server library, to implement Query API websocket subscriptions
- The [Catch](https://github.com/philsquared/Catch) automated test framework, for unit testing
- A few of the [Boost C++ Libraries](http://www.boost.org/)

## Preparation

The following instructions describe how to prepare these external dependencies when building this software on Windows with Visual Studio 2013.
On Linux and other platforms, the steps to install these dependencies vary slightly.

### CMake

1. Download and install a recent [CMake stable release](https://cmake.org/download/#latest) for your platform  
   Notes:
   - C++ REST SDK currently requires CMake 3.9 or higher
   - Some CMake modules derived from third-party sources are included in the [third_party/cmake](../Development/third_party/cmake) directory

### Boost C++ Libraries

1. Download a [recent release](http://www.boost.org/users/download/)  
   Note: Several Boost releases have been tested, including Version 1.65.1 (current release) and Version 1.54.0
2. Expand the archive so that, for example, the boost\_1\_65\_1 directory is at the same level as the nmos-cpp directory
3. Build and stage (or install) the following Boost libraries for your platform/toolset:
   - date_time
   - regex
   - system

For example:  
```
b2 toolset=msvc-12.0 --prefix=. --with-date_time --with-regex --with-system --stagedir=x64 stage address-model=64
```

### WebSocket++

A copy of the header-only WebSocket++ v0.5.1 is included in the C++ REST SDK, so a separate installation is not necessary.
Note: WebSocket++ v0.7.0 (latest release) has also been tested.

### C++ REST SDK

1. Download the [v2.10.0-nmos-cpp archive](https://github.com/garethsb-sony/cpprestsdk/archive/v2.10.0-nmos-cpp.zip) from GitHub
2. Expand the archive so that, for example, the cpprestsdk directory is at the same level as the nmos-cpp directory
3. Use CMake to configure for your platform
   - Set CMake variables to control building C++ REST SDK:
     - Set ``CPPREST_PPLX_IMPL`` (STRING) to ``winpplx`` to solve a performance issue
     - Set ``CMAKE_CONFIGURATION_TYPES`` (STRING) to ``Debug;Release`` to build only those configurations
     - Set ``Boost_USE_STATIC_LIBS`` (BOOL) to ``1`` (true)
   - If CMake cannot find it automatically, set hints for [finding Boost](https://cmake.org/cmake/help/latest/module/FindBoost.html), for example:
     - Set ``BOOST_INCLUDEDIR`` (PATH) to the appropriate full path, e.g. ``.../boost_1_65_1`` to match the suggested ``b2`` command
     - Set ``BOOST_LIBRARYDIR`` (PATH) to the appropriate full path, e.g. ``.../boost_1_65_1/x64/lib`` to match the suggested ``b2`` command
4. Use CMake to generate project files  
   The "Visual Studio 12 2013 Win64" generator has been tested

### Bonjour SDK for Windows

1. Download the [Bonjour SDK for Windows v3.0](https://developer.apple.com/download/more/?=Bonjour%20SDK%20for%20Windows) from Apple Developer Downloads
2. Run bonjoursdksetup.exe
3. **Choose to install the SDK so that the Bonjour SDK directory is at the same level as the nmos-cpp directory**  
   The critical files for building this software are:
   - Bonjour SDK\Include\dns_sd.h
   - Bonjour SDK\Lib\x64\dnssd.lib
4. The Bonjour service itself needs to be installed on the system where this software is run  
   The installer is included in the SDK:
   - Bonjour SDK\Installer\Bonjour64.msi

### Catch

A copy of the single header version (v1.10.0) is included in the [third_party/catch](../Development/third_party/catch) directory.

# What Next?

These [instructions](Getting-Started.md) explain how to build nmos-cpp itself, run the test suite, and try out the registry and node applications.
