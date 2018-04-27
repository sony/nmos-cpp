# Dependencies

This software is being developed for multiple platforms (Windows, Mac OS X, Linux) using portable C++.

The codebase utilizes a number of great open-source projects (licenses vary).

- The [CMake](https://cmake.org/) build tools
- The [C++ REST SDK](https://github.com/Microsoft/cpprestsdk), for client-server communication over REST APIs
  - This library incorporates some third-party material including WebSocket++, and also relies on e.g. some of the Boost C++ Libraries and OpenSSL.
- For DNS Service Discovery (DNS-SD), the [Bonjour SDK](https://developer.apple.com/bonjour/) (on Windows; it is known as [mDNSResponder](https://opensource.apple.com/tarballs/mDNSResponder/) on Linux)
- The [WebSocket++](https://github.com/zaphoyd/websocketpp) header-only C++ websocket client/server library, to implement Query API websocket subscriptions
- The [Catch](https://github.com/philsquared/Catch) automated test framework, for unit testing
- A few of the [Boost C++ Libraries](http://www.boost.org/)

## Preparation

The following instructions describe how to prepare these external dependencies when building this software.

### CMake

1. Download and install a recent [CMake stable release](https://cmake.org/download/#latest) for your platform  
   Notes:
   - C++ REST SDK currently requires CMake 3.9 or higher, and using Boost 1.66.0 or higher requires CMake 3.11
   - Some CMake modules derived from third-party sources are included in the [third_party/cmake](../Development/third_party/cmake) directory

### Boost C++ Libraries

1. Download a [recent release](http://www.boost.org/users/download/)  
   Note: Several Boost releases have been tested, including Version 1.67.0 (current release) and Version 1.54.0
2. Expand the archive so that, for example, the boost\_1\_67\_0 directory is at the same level as the nmos-cpp directory
3. Build and stage (or install) the following Boost libraries for your platform/toolset:
   - date_time
   - regex
   - system
   - thread

For example, on Windows, for Visual Studio 2013:
```
bootstrap
b2 toolset=msvc-12.0 --prefix=. --with-date_time --with-regex --with-system --with-thread --stagedir=x64 stage address-model=64
```

### WebSocket++

A copy of the header-only WebSocket++ v0.5.1 is included in the C++ REST SDK, so a separate installation is not necessary.
Note: WebSocket++ v0.7.0 (latest release) has also been tested.

### OpenSSL

The C++ REST SDK depends on [OpenSSL](https://www.openssl.org/) (to implement secure HTTP and/or secure WebSockets).
It is compatible with the OpenSSL 1.0.2 series, the Long Term Support (LTS) release. OpenSSL 1.1.0 is quite different, and not currently supported.

1. On Windows, an installer can be downloaded from [Shining Light Productions - Win32 OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)  
   The [Win64 OpenSSL v1.0.2o Light](https://slproweb.com/download/Win64OpenSSL_Light-1_0_2o.exe) installer has been tested

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
4. Use CMake to generate project files  
   On Windows, the "Visual Studio 12 2013 Win64" generator has been tested

For example, on Windows, for Visual Studio 2013:
```
cmake ^
  -G "Visual Studio 12 2013 Win64" ^
  -DCPPREST_PPLX_IMPL:STRING="winpplx" ^
  -DCPPREST_EXCLUDE_COMPRESSION:BOOL="1" ^
  -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release" ^
  -DBoost_USE_STATIC_LIBS:BOOL="1" ^
  -DBOOST_INCLUDEDIR:PATH=".../boost_1_67_0" ^
  -DBOOST_LIBRARYDIR:PATH=".../boost_1_67_0/x64/lib" ^
  -DWERROR:BOOL="0"
```

### DNS Service Discovery

**Windows**

On Windows:

1. Download the [Bonjour SDK for Windows v3.0](https://developer.apple.com/download/more/?=Bonjour%20SDK%20for%20Windows) from Apple Developer Downloads
2. Run bonjoursdksetup.exe
3. **Choose to install the SDK so that the Bonjour SDK directory is at the same level as the nmos-cpp directory**  
   The critical files for building this software are:
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

# What Next?

These [instructions](Getting-Started.md) explain how to build nmos-cpp itself, run the test suite, and try out the registry and node applications.
