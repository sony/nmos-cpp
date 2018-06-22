# Dependencies

This software is being developed for multiple platforms (Windows, Mac OS X, Linux) using portable C++.

The codebase utilizes a number of great open-source projects (licenses vary).

- The [CMake](https://cmake.org/) build tools
- The [C++ REST SDK](https://github.com/Microsoft/cpprestsdk), for client-server communication over REST APIs
  - This library incorporates some third-party material including WebSocket++, and also relies on e.g. some of the Boost C++ Libraries and OpenSSL.
- For JSON Schema validation, the [Modern C++ JSON schema validator](https://github.com/pboettch/json-schema-validator) library, which is implemented on top of [JSON for Modern C++](https://github.com/nlohmann/json)
- For DNS Service Discovery (DNS-SD), the [Bonjour SDK](https://developer.apple.com/bonjour/) (on Windows; it is known as [mDNSResponder](https://opensource.apple.com/tarballs/mDNSResponder/) on Linux)
- The [WebSocket++](https://github.com/zaphoyd/websocketpp) header-only C++ websocket client/server library, to implement Query API websocket subscriptions
- The [Catch](https://github.com/philsquared/Catch) automated test framework, for unit testing
- A few of the [Boost C++ Libraries](http://www.boost.org/)

## Preparation

The following instructions describe how to prepare these external dependencies when building this software.

On Windows, Visual Studio is required. Visual Studio 2015 is the primary development environment; more recent toolchains should also be supported.

On Linux, ``g++`` (the GNU project C++ compiler) is supported; the GCC 4.7 release series has been tested, although a more recent compiler is to be recommended!

### CMake

1. Download and install a recent [CMake stable release](https://cmake.org/download/#latest) for your platform  
   Notes:
   - C++ REST SDK currently requires CMake 3.9 or higher, and using Boost 1.66.0 or higher requires CMake 3.11
   - Pre-built binary distributions are available for many platforms
   - Some CMake modules derived from third-party sources are included in the [third_party/cmake](../Development/third_party/cmake) directory

### Boost C++ Libraries

1. Download a [recent release](http://www.boost.org/users/download/)  
   Notes:
   - Several Boost releases have been tested, including Version 1.67.0 (current release) and Version 1.54.0
   - On Linux distributions, a Boost libraries package may already be installed, e.g. Ubuntu 14.04 LTS (long-term support) has Version 1.54.0
2. Expand the archive so that, for example, the boost\_1\_67\_0 directory is at the same level as the nmos-cpp directory
3. Build and stage (or install) the following Boost libraries for your platform/toolset:
   - chrono
   - date_time
   - regex
   - system
   - thread

For example, on Windows, for Visual Studio 2015:
```
bootstrap
b2 toolset=msvc-14.0 --prefix=. --with-chrono --with-date_time --with-regex --with-system --with-thread --stagedir=x64 stage address-model=64
```

### WebSocket++

A copy of the header-only WebSocket++ v0.5.1 is included within the C++ REST SDK source tree, so a separate installation is not necessary.
Note: WebSocket++ v0.7.0 (latest release) has also been tested.

(The [Getting Started](Getting-Started.md) instructions explain how to set ``WEBSOCKETPP_INCLUDE_DIR`` in order to use the included version when building nmos-cpp.)

### OpenSSL

The C++ REST SDK depends on [OpenSSL](https://www.openssl.org/) (to implement secure HTTP and/or secure WebSockets).
It is compatible with the OpenSSL 1.0 API, so the 1.0.2 Long Term Support (LTS) release is recommended. OpenSSL 1.1 is quite different, and not currently supported.

1. Download and install a recent release
   Notes:
   - On Windows, an installer can be downloaded from [Shining Light Productions - Win32 OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)  
     The [Win64 OpenSSL v1.0.2o Light](https://slproweb.com/download/Win64OpenSSL_Light-1_0_2o.exe) installer has been tested
   - On Linux distributions, an OpenSSL package may already be installed, e.g. Ubuntu 14.04 LTS has version 1.01f

### C++ REST SDK

1. Download the [v2.10.2-nmos-cpp archive](https://github.com/garethsb-sony/cpprestsdk/archive/v2.10.2-nmos-cpp.zip) from GitHub
2. Expand the archive so that, for example, the cpprestsdk directory is at the same level as the nmos-cpp directory
3. Use CMake to configure for your platform
   - If you're not familiar with CMake, the CMake GUI may be helpful
   - Set CMake variables to control building C++ REST SDK
   - On Windows:
     - Set ``CPPREST_PPLX_IMPL`` (STRING) to ``winpplx`` to solve a performance issue
     - Set ``CPPREST_EXCLUDE_COMPRESSION`` (BOOL) to ``1`` (true) to avoid [zlib](https://zlib.net/) being required
     - Set ``CMAKE_CONFIGURATION_TYPES`` (STRING) to ``Debug;Release`` to build only those configurations
     - Set ``Boost_USE_STATIC_LIBS`` (BOOL) to ``1`` (true)
   - If CMake cannot find it automatically, set hints for [finding Boost](https://cmake.org/cmake/help/latest/module/FindBoost.html), for example:
     - Set ``BOOST_INCLUDEDIR`` (PATH) to the appropriate full path, e.g. ``.../boost_1_67_0`` to match the suggested ``b2`` command
     - Set ``BOOST_LIBRARYDIR`` (PATH) to the appropriate full path, e.g. ``.../boost_1_67_0/x64/lib`` to match the suggested ``b2`` command
   - Due to interactions with other dependencies, it may also be necessary to explicitly set ``WERROR`` (BOOL) to ``0`` so that compiler warnings are not treated as errors
4. Use CMake to generate project files  
   On Windows, the "Visual Studio 14 2015 Win64" generator has been tested

For example, on Windows, for Visual Studio 2015:
```
cd .../Release
mkdir build
cd build
cmake .. ^
  -G "Visual Studio 14 2015 Win64" ^
  -DCPPREST_PPLX_IMPL:STRING="winpplx" ^
  -DCPPREST_EXCLUDE_COMPRESSION:BOOL="1" ^
  -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release" ^
  -DBoost_USE_STATIC_LIBS:BOOL="1" ^
  -DBOOST_INCLUDEDIR:PATH=".../boost_1_67_0" ^
  -DBOOST_LIBRARYDIR:PATH=".../boost_1_67_0/x64/lib" ^
  -DWERROR:BOOL="0"
```

For example, on Linux, using the default toolchain and dependencies:

```
cd .../Release
mkdir build
cd build
cmake .. ^
  -DCMAKE_BUILD_TYPE:STRING="Release" ^
  -DWERROR:BOOL="0"
make
sudo make install
```

### Modern C++ JSON schema validator

A copy of the source code necessary to use this library is included in the [third_party/nlohmann](../Development/third_party/nlohmann) directory.
No installation is necessary.

### DNS Service Discovery

**Windows**

On Windows:

1. Download the [Bonjour SDK for Windows v3.0](https://developer.apple.com/download/more/?=Bonjour%20SDK%20for%20Windows) from Apple Developer Downloads
2. Run bonjoursdksetup.exe, which will install the necessary files in e.g. *C:\Program Files\Bonjour SDK*
3. The critical files for building this software are:
   - Bonjour SDK\Include\dns_sd.h
   - Bonjour SDK\Lib\x64\dnssd.lib
4. The Bonjour service itself needs to be installed on the system where this software is run  
   The installer is included in the SDK:
   - Bonjour SDK\Installer\Bonjour64.msi

**Linux**

The [Avahi](https://www.avahi.org/) project provides a DNS-SD daemon for Linux, and the *avahi-compat-libdns_sd* library which enables applications to use the original Bonjour *dns_sd.h* API to communicate with the Avahi daemon.

Alternatively, [Apple's mDNSResponder (also known as ``mdnsd``)](https://opensource.apple.com/tarballs/mDNSResponder/) can itself be built from source for Linux, and this is the currently tested approach. Version 878.30.4 (latest release) has been tested.

The ``mDNSResponder`` build instructions are quite straightforward. For example, to build and install:
```
cd mDNSResponder-878.30.4/mDNSPosix
make os=linux
sudo make os=linux install
```

Notes:
- The [poll-rather-than-select.patch](../Development/third_party/mDNSResponder/poll-rather-than-select.patch) found in this repository is recommended to build the ``libdns_sd.lib`` client-side library to communicate successfully with the ``mdnsd`` daemon on Linux hosts where (even moderately) huge numbers of file descriptors may be in use.
  ```
  patch -d mDNSResponder-878.30.4/ -p1 <poll-rather-than-select.patch
  ```
- On systems with IPv6 disabled, the default build of ``mdnsd`` may immediately stop (when run with ``-debug``, the error ``socket AF_INET6: Address family not supported by protocol`` is reported). Prefixing the ``make`` command above with ``HAVE_IPV6=0`` solves this issue at the cost of repeated warnings from the preprocessor during compilation.

### Catch

A copy of the single header version (v1.10.0) is included in the [third_party/catch](../Development/third_party/catch) directory.
No installation is necessary.

# What Next?

These [instructions](Getting-Started.md) explain how to build nmos-cpp itself, run the test suite, and try out the registry and node applications.
