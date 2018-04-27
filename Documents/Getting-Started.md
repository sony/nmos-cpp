# Getting Started

The following instructions describe how to set up and build this software on Windows with Visual Studio 2013.
On Linux and other platforms, the steps vary slightly.

## Preparation

0. This software utilizes a number of great open-source projects  
   Set up these [external dependencies](Dependencies.md#preparation) before proceeding
1. Use CMake to configure for your platform
   - If you're not familiar with CMake, the CMake GUI may be helpful
   - Set the CMake source directory to the [Development](../Development) directory in the nmos-cpp source tree
   - Set the CMake build directory to an appropriate location, e.g. ``.../Development/build``
   - Set CMake variables to control building nmos-cpp
   - On Windows:
     - Set ``CMAKE_CONFIGURATION_TYPES`` (STRING) to ``Debug;Release`` to build only those configurations
     - Set ``Boost_USE_STATIC_LIBS`` (BOOL) to ``1`` (true)
   - If CMake cannot find it automatically, set hints for [finding Boost](https://cmake.org/cmake/help/latest/module/FindBoost.html), for example:
     - Set ``BOOST_INCLUDEDIR`` (PATH) to the appropriate full path, e.g. ``.../boost_1_67_0`` to match the suggested ``b2`` command
     - Set ``BOOST_LIBRARYDIR`` (PATH) to the appropriate full path, e.g. ``.../boost_1_67_0/x64/lib`` to match the suggested ``b2`` command
   - If CMake cannot find them automatically, set hints for finding the C++ REST SDK and WebSocket++, for example:
     - Set ``cpprestsdk_DIR`` (PATH) to the location of the installed cpprestsdk-config.cmake
     - *Either* set ``websocketpp_DIR`` (PATH) to the location of the installed websocketpp-config.cmake
     - *Or* set ``WEBSOCKETPP_INCLUDE_DIR`` (PATH) to the location of the WebSocket++ include files, e.g. ``.../Release/libs/websocketpp`` within the C++ REST SDK source tree
2. Use CMake to generate project files  
   The "Visual Studio 12 2013 Win64" generator has been tested

For example, on Windows, for Visual Studio 2013:
```
cmake ^
  -G "Visual Studio 12 2013 Win64" ^
  -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release" ^
  -DBoost_USE_STATIC_LIBS:BOOL="1" ^
  -DBOOST_INCLUDEDIR:PATH=".../boost_1_67_0" ^
  -DBOOST_LIBRARYDIR:PATH=".../boost_1_67_0/x64/lib" ^
  -DWEBSOCKETPP_INCLUDE_DIR:PATH=".../Release/libs/websocketpp"
```

## Build

On Windows, open and build the generated nmos-cpp Visual Studio Solution.

## Run Tests

All the tests are currently packaged into a single test suite, as the **nmos-cpp-registry-test** application.
This may be run automatically by building RUN_TESTS, but note that to see the output of any failed tests,
it is necessary to set ``CTEST_OUTPUT_ON_FAILURE`` to ``1`` in the environment first.

The output should conclude something like this:

```
  100% tests passed, 0 tests failed out of 29
  
  Total Test time (real) =  38.08 sec
```

Note: On Windows, the correct configuration of the C++ REST SDK library (e.g. cpprestsdk120_2_10.dll or cpprest120_2_10d.dll) needs to be on the ``PATH`` or copied into the output directory.

## What Next?

The [tutorial](Tutorial.md) explains how to run the NMOS Registry and some example NMOS Nodes provided by nmos-cpp.
