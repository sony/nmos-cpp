# Dependencies

This software is being developed for multiple platforms (Windows, Mac OS X, Linux) using portable C++.

The codebase utilizes a number of great open-source projects (licenses vary).

- The [C++ REST SDK](https://github.com/Microsoft/cpprestsdk), for client-server communication over REST APIs
    - This library incorporates some third party material including WebSocket++, and also relies on e.g. some of the Boost C++ Libraries and OpenSSL.
- The [Bonjour SDK](https://developer.apple.com/bonjour/), to implement DNS Service Discovery (DNS-SD)
- The [WebSocket++](https://github.com/zaphoyd/websocketpp) header-only C++ websocket client/server library, to implement Query API websocket subscriptions
- The [Catch](https://github.com/philsquared/Catch) automated test framework, for unit testing
- A few of the [Boost C++ Libraries](http://www.boost.org/)

## Preparation

The following instructions describe how to prepare these external dependencies when building this software on Windows with Visual Studio 2013.

This procedure will be updated when Mac OS X and Linux project/build files are added *soon*.

For Windows, the first two dependencies need to be downloaded and installed manually.

### C++ REST SDK

1. Download the [v2.9.1 archive](https://github.com/Microsoft/cpprestsdk/archive/v2.9.1.zip) from GitHub
2. **Expand the archive so that the cpprestsdk directory is at the same level as the nmos-cpp directory**
3. Overwrite cpprestsdk\Release\src\build\vs12\casablanca120.vcxproj with the [patched file](../Downloads/cpprestsdk-patches/Release/src/build/vs12/casablanca120.vcxproj)  
   This simply adds CPPREST_FORCE_PPLX to the preprocessor definitions to solve a performance issue

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

### WebSocket++

A copy of WebSocket++ v0.5.1 is included in the C++ REST SDK, so no installation is necessary.

### Catch

A copy of the single header version (v1.0 build 53) is included in this repository.

### Boost C++ Libraries

The required Boost C++ Libraries (Version 1.58.0) header files are installed by the C++ REST SDK using NuGet.
