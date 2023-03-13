# Dependencies

This software is being developed for multiple platforms (Windows, Mac OS X, Linux) using portable C++.

The codebase utilizes a number of great open-source projects (licenses vary).
More details are given below.

- The [CMake](https://cmake.org/) build tools
- The [Conan](https://conan.io) package manager
- A few of the [Boost C++ Libraries](http://www.boost.org/)
- The [C++ REST SDK](https://github.com/Microsoft/cpprestsdk), for client-server communication over REST APIs
  - This library incorporates some third-party material including WebSocket++, and also relies on e.g. some of the Boost C++ Libraries and [OpenSSL](https://www.openssl.org/).
- The [WebSocket++](https://github.com/zaphoyd/websocketpp) header-only C++ websocket client/server library, to implement Query API websocket subscriptions
- For JSON Schema validation, the [Modern C++ JSON schema validator](https://github.com/pboettch/json-schema-validator) library, which is implemented on top of [JSON for Modern C++](https://github.com/nlohmann/json)
- For DNS Service Discovery (DNS-SD), the [Bonjour SDK](https://developer.apple.com/bonjour/) on Windows, and on Linux either [Avahi](https://www.avahi.org/) or Apple's [mDNSResponder](https://opensource.apple.com/tarballs/mDNSResponder/) (another name for Bonjour)
- The [Catch](https://github.com/philsquared/Catch) automated test framework, for unit testing

## Preparation

The following instructions describe how to prepare these external dependencies when building this software.

On Windows, Visual Studio 2015 or higher is required.

On Linux, ``g++`` (the GNU project C++ compiler) is supported; the GCC 4.8 release series is tested, although a more recent compiler is to be recommended!

Notes:
- **Visual Studio 2013 is not supported**, because it does not implement C++11 [thread-safe function-local static initialization](https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables).
- For the same reason, Visual Studio 2015 onwards with ``/Zc:threadSafeInit-``, and GCC with ``--fno-threadsafe-statics``, are also not supported.

Specific instructions for [cross-compiling for Raspberry Pi](Raspberry-Pi.md) are also provided.

### CMake

1. Download and install a recent [CMake stable release](https://cmake.org/download/#latest) for your platform  
   Notes:
   - Currently, CMake 3.17 or higher is required; version 3.24.2 (latest release at the time) has been tested
   - Pre-built binary distributions are available for many platforms
   - On Linux distributions, e.g. Ubuntu 14.04 LTS (long-term support), the pre-built binary version available via ``apt-get`` may be too out-of-date  
     Fetch, build and install a suitable version:  
     ```sh
     wget "https://cmake.org/files/v3.24/cmake-3.24.2.tar.gz"
     tar -zxvf cmake-3.24.2.tar.gz
     cd cmake-3.24.2
     ./bootstrap
     make
     sudo make install
     cd ..
     ```
   - Some CMake modules derived from third-party sources are included in the [third_party/cmake](../Development/third_party/cmake) directory

### Conan

By default nmos-cpp uses [Conan](https://conan.io) to download most of its dependencies.

1. Install Python 3 if necessary  
   Note: The Python scripts directory needs to be added to the `PATH`, so the Conan executable can be found
2. Install Conan using `pip install conan~=1.47`
   Notes:
   - On some platforms with Python 2 and Python 3 both installed this may need to be `pip3 install conan~=1.47`
   - Currently, Conan 1.47 or higher (and lower than version 2.0) is required; version 1.59.0 has been tested
   - Conan evolves fairly quickly, so it's worth running `pip install --upgrade conan` regularly
   - By default [Conan assumes semver compatibility](https://docs.conan.io/en/1.42/creating_packages/define_abi_compatibility.html#versioning-schema).
     Boost and other C++ libraries do not meet this expectation and break ABI compatibility between e.g. minor versions.
     Unfortunately, the recipes in Conan Center Index do not generally customize their `package_id` method to take this into account.
     Therefore it is strongly recommended to change Conan's default package id mode to `minor_mode` or a stricter mode such as `recipe_revision_mode`.
     ```sh
     conan config set general.default_package_id_mode=minor_mode
     ```
3. Install a [DNS Service Discovery](#dns-service-discovery) implementation, since this isn't currently handled by Conan

Now follow the [Getting Started](Getting-Started.md) instructions directly. Conan is used to download the rest of the dependencies.

If you prefer not to use Conan, you must install Boost, WebSocket++, OpenSSL and C++ REST SDK as detailed below then call CMake with `-DNMOS_CPP_USE_CONAN:BOOL="0"` when building nmos-cpp.

### Boost C++ Libraries

If using Conan, this section can be skipped.
<details>
<summary>If not using Conan...</summary>

1. Download a [recent release](http://www.boost.org/users/download/)  
   Notes:
   - Several Boost releases have been tested, including Version 1.80.0 (latest release at the time) and Version 1.54.0
   - On Linux distributions, a Boost libraries package may already be installed, e.g. Ubuntu 14.04 LTS has Version 1.54.0
2. Expand the archive so that, for example, the boost\_1\_80\_0 directory is at the same level as the nmos-cpp directory
3. Build and stage (or install) the following Boost libraries for your platform/toolset:
   - atomic
   - chrono
   - date_time
   - regex
   - system
   - thread

For example, on Windows, for Visual Studio 2019:
```sh
bootstrap
b2 toolset=msvc-14.2 ^
  --prefix=. ^
  --with-atomic ^
  --with-chrono ^
  --with-date_time ^
  --with-regex ^
  --with-system ^
  --with-thread ^
  --stagedir=x64 ^
  stage ^
  address-model=64
```

For example, on Linux:
```sh
./bootstrap.sh
sudo ./b2 \
  '--prefix=`pwd`' \
  --with-atomic \
  --with-chrono \
  --with-date_time \
  --with-filesystem \
  --with-random \
  --with-regex \
  --with-system \
  --with-thread \
  --stagedir=. \
  stage
```

</details>

### C++ REST SDK

If using Conan, this section can be skipped.
<details>
<summary>If not using Conan...</summary>

1. Get the source code
   - Clone the [repo](https://github.com/Microsoft/cpprestsdk/) and its submodules, and check out the v2.10.18 tag  
     The ``git clone`` command option ``--recurse-submodules`` (formerly ``--recursive``) simplifies [cloning a project with submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules#_cloning_submodules).  
     For example:
     ```
     git clone --recurse-submodules --branch v2.10.18 https://github.com/Microsoft/cpprestsdk <home-dir>/cpprestsdk
     ```
     Note: The downloadable archives created by GitHub cannot be used on their own since they don't include submodules.
2. Use CMake to configure for your platform
   - If you're not familiar with CMake, the CMake GUI may be helpful
     - Set the CMake source directory to the Release directory in the cpprestsdk source tree
     - Set the CMake build directory to an appropriate location, e.g. *``<home-dir>``*``/cpprestsdk/Release/build``
   - Set CMake variables to control building C++ REST SDK
   - On Windows:
     - Set ``CPPREST_PPLX_IMPL`` (STRING) to ``winpplx`` to solve a performance issue
     - Set ``CPPREST_EXCLUDE_COMPRESSION`` (BOOL) to ``1`` (true) to avoid [zlib](https://zlib.net/) being required
     - Set ``CMAKE_CONFIGURATION_TYPES`` (STRING) to ``Debug;Release`` to build only those configurations
     - Set ``Boost_USE_STATIC_LIBS`` (BOOL) to ``1`` (true)
   - If CMake cannot find it automatically, set hints for [finding Boost](https://cmake.org/cmake/help/latest/module/FindBoost.html), for example:
     - *Either* set ``Boost_DIR`` (PATH) to the location of the installed BoostConfig.cmake (since Boost 1.70.0)
     - *Or* set ``BOOST_INCLUDEDIR`` (PATH) and ``BOOST_LIBRARYDIR`` (PATH) to the appropriate full paths, e.g. *``<home-dir>``*``/boost_1_80_0``
       and *``<home-dir>``*``/boost_1_80_0/x64/lib`` respectively to match the suggested ``b2`` command
   - Due to interactions with other dependencies, it may also be necessary to explicitly set ``WERROR`` (BOOL) to ``0`` so that compiler warnings are not treated as errors
   - To speed up the build by omitting the C++ REST SDK sample apps and test suite, set ``BUILD_SAMPLES`` and ``BUILD_TESTS`` (BOOL) to ``0`` (false)
3. Use CMake to generate build/project files, and then build *and* install  
   "Visual Studio 14 2015 Win64" and more recent Visual Studio generators have been tested

**Windows**

For example, for Visual Studio 2019:
```
cd <home-dir>\cpprestsdk\Release
mkdir build
cd build
cmake .. ^
  -G "Visual Studio 16 2019" ^
  -DCPPREST_PPLX_IMPL:STRING="winpplx" ^
  -DCPPREST_EXCLUDE_COMPRESSION:BOOL="1" ^
  -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release" ^
  -DBoost_USE_STATIC_LIBS:BOOL="1" ^
  -DBOOST_INCLUDEDIR:PATH="<home-dir>/boost_1_80_0" ^
  -DBOOST_LIBRARYDIR:PATH="<home-dir>/boost_1_80_0/x64/lib" ^
  -DWERROR:BOOL="0" ^
  -DBUILD_SAMPLES:BOOL="0" ^
  -DBUILD_TESTS:BOOL="0"
```

Then, open and build the INSTALL project in the generated cpprestsdk Visual Studio Solution.

Note: Depending on the current user permissions, Visual Studio may need to be run with administrator privileges in order to install C++ REST SDK.

Or on the Developer command line:
```sh
msbuild cpprestsdk.sln /p:Configuration=<Debug-or-Release>
msbuild INSTALL.vcxproj /p:Configuration=<Debug-or-Release>
```

Note: With the CMake configuration options for C++ REST SDK described above, **nmos-cpp** apps themselves may need to be run with administrator privileges on Windows.
This is because the C++ REST SDK implementation uses Windows HTTP Services (WinHTTP) by default, which enforces this requirement when using the "*" wildcard to listen on all interfaces.
Administrator privileges are not required if C++ REST SDK is built with ``CPPREST_HTTP_LISTENER_IMPL`` (STRING) set to ``asio`` (and for consistency ``CPPREST_HTTP_CLIENT_IMPL`` (STRING) also set to ``asio``).

**Linux**

For example, using the default toolchain and dependencies:

```sh
cd <home-dir>/cpprestsdk/Release
mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE:STRING="<Debug-or-Release>" \
  -DWERROR:BOOL="0" \
  -DBUILD_SAMPLES:BOOL="0" \
  -DBUILD_TESTS:BOOL="0"
make
sudo make install
```

(To speed up the build, the make ``-j`` option can be used to utilise multiple processor cores, e.g. ``make -j 4``.)

</details>

### WebSocket++

If using Conan, this section can be skipped.
<details>
<summary>If not using Conan...</summary>

WebSocket++ v0.8.2 (latest release at the time) is included as a submodule within the C++ REST SDK source tree, so a separate installation is not necessary.
Note: WebSocket++ v0.5.1 and v0.7.0 have also been tested.

(The [Getting Started](Getting-Started.md) instructions explain how to set ``WEBSOCKETPP_INCLUDE_DIR`` in order to use the included version when building nmos-cpp.)

</details>

### OpenSSL

If using Conan, this section can be skipped.
<details>
<summary>If not using Conan...</summary>

The C++ REST SDK depends on [OpenSSL](https://www.openssl.org/) (to implement secure HTTP and/or secure WebSockets).
It is compatible with the OpenSSL 1.1 API, so the 1.1.1 Long Term Support (LTS) release is recommended.
It is also possible to use OpenSSL 1.0, but the OpenSSL team announced that [users of that release are strongly advised to upgrade to OpenSSL 1.1.1](https://www.openssl.org/blog/blog/2018/09/11/release111/).

1. Download and install a recent release
   Notes:
   - On Windows, an installer can be downloaded from [Shining Light Productions - Win32 OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)  
     The Win64 OpenSSL v1.1.1s installer (latest release at the time) has been tested
   - On Linux distributions, an OpenSSL package may already be available  
     The Ubuntu team announced an [OpenSSL 1.1.1 stable release update (SRU) for Ubuntu 18.04 LTS](https://lists.ubuntu.com/archives/ubuntu-devel/2018-December/040567.html)

</details>

### Modern C++ JSON schema validator

If using Conan, this section can be skipped.
<details>
<summary>If not using Conan...</summary>

A copy of the source code necessary to use this library is included in the [third_party/nlohmann](../Development/third_party/nlohmann) directory.
No installation is necessary.

</details>

### DNS Service Discovery

**Windows**

On Windows:

1. Download the [Bonjour SDK for Windows v3.0](https://developer.apple.com/download/more/?=Bonjour%20SDK%20for%20Windows) from Apple Developer Downloads
2. Run bonjoursdksetup.exe, which will install the necessary files in e.g. *C:\Program Files\Bonjour SDK*
3. The critical files for building this software are:
   - *Bonjour SDK\Include\dns_sd.h*
   - *Bonjour SDK\Lib\x64\dnssd.lib*
4. The Bonjour service itself needs to be installed on the system where this software is run  
   The installer is included in the SDK:
   - *Bonjour SDK\Installer\Bonjour64.msi*

**Linux**

The [Avahi](https://www.avahi.org/) project provides a DNS-SD daemon for Linux, and the *avahi-compat-libdns_sd* library which enables applications to use the original Bonjour *dns_sd.h* API to communicate with the Avahi daemon.

For example, to install the Bonjour compatibility library, Name Service Switch module, and Avahi tools on Ubuntu:
```sh
sudo apt-get install -f libavahi-compat-libdnssd-dev libnss-mdns avahi-utils
```

Alternatively, [Apple's mDNSResponder (also known as ``mdnsd``)](https://github.com/apple-oss-distributions/mDNSResponder) can itself be built from source for Linux. Version 878.200.35 has been tested.

The ``mDNSResponder`` build instructions are quite straightforward. For example, to build and install:
```sh
cd mDNSPosix
make os=linux
sudo make os=linux install
```

(The [Getting Started](Getting-Started.md) instructions explain how to set ``NMOS_CPP_USE_AVAHI`` (BOOL) to ``0`` (false) in order to use ``mDNSResponder`` when building nmos-cpp.)

Notes:
- The [unicast](../Development/third_party/mDNSResponder/unicast.patch) and [permit-over-long-service-types](../Development/third_party/mDNSResponder/permit-over-long-service-types.patch) patches found in this repository is recommended to build the ``mdnsd`` daemon on Linux in order to support unicast DNS-SD.
  ```sh
  patch -p1 <unicast.patch
  patch -p1 <permit-over-long-service-types.patch
  ```
- The [poll-rather-than-select](../Development/third_party/mDNSResponder/poll-rather-than-select.patch) patch found in this repository is recommended to build the ``libdns_sd.lib`` client-side library to communicate successfully with the ``mdnsd`` daemon on Linux hosts where (even moderately) huge numbers of file descriptors may be in use.
  ```sh
  patch -p1 <poll-rather-than-select.patch
  ```
- On systems with IPv6 disabled, the default build of ``mdnsd`` may immediately stop (when run with ``-debug``, the error ``socket AF_INET6: Address family not supported by protocol`` is reported). Prefixing the ``make`` command above with ``HAVE_IPV6=0`` solves this issue at the cost of repeated warnings from the preprocessor during compilation.

### Catch

A copy of the single header version (v1.10.0) is included in the [third_party/catch](../Development/third_party/catch) directory.
No installation is necessary.

# What Next?

These [instructions](Getting-Started.md) explain how to build nmos-cpp itself, run the test suite, and try out the registry and node applications.
