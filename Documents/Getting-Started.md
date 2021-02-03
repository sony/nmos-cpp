# Getting Started

The following instructions describe how to set up and build this software.

The test platforms include Windows with Visual Studio 2017 and 2019, and Linux with GCC 4.8 and GCC 7.5.
Specific instructions for [cross-compiling for Raspberry Pi](Raspberry-Pi.md) are also provided.

Notes:
- **Visual Studio 2013 is not supported**, because it does not implement C++11 [thread-safe function-local static initialization](https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables).
- For the same reason, Visual Studio 2015 onwards with ``/Zc:threadSafeInit-``, and GCC with ``--fno-threadsafe-statics``, are also not supported.

## Preparation

0. This software utilizes a number of great open-source projects  
   Set up these [external dependencies](Dependencies.md#preparation) before proceeding
1. If using Visual Studio 2017 or later and Conan, you can simply use the "Open Folder" option with the [Development](../Development) directory and skip the rest of this section.
2. Use CMake to configure for your platform
   - If you're not familiar with CMake, the CMake GUI may be helpful
   - Set the CMake source directory to the [Development](../Development) directory in the nmos-cpp source tree
   - Set the CMake build directory to an appropriate location, e.g. *``<home-dir>``*``/nmos-cpp/Development/build``
   - Set CMake variables to control building nmos-cpp
   - On Windows:
     - Set ``CMAKE_CONFIGURATION_TYPES`` (STRING) to ``Debug;Release`` to build only those configurations
     - If not using Conan
        - Set ``Boost_USE_STATIC_LIBS`` (BOOL) to ``1`` (true)
   - If not using Conan
     - If CMake cannot find it automatically, set hints for [finding Boost](https://cmake.org/cmake/help/latest/module/FindBoost.html), for example:
       - Set ``BOOST_INCLUDEDIR`` (PATH) to the appropriate full path, e.g. *``<home-dir>``*``/boost_1_75_0`` to match the suggested ``b2`` command
       - Set ``BOOST_LIBRARYDIR`` (PATH) to the appropriate full path, e.g. *``<home-dir>``*``/boost_1_75_0/x64/lib`` to match the suggested ``b2`` command
     - If CMake cannot find them automatically, set hints for finding the C++ REST SDK and WebSocket++, for example:
       - Set ``cpprestsdk_DIR`` (PATH) to the location of the installed cpprestsdk-config.cmake
       - *Either* set ``websocketpp_DIR`` (PATH) to the location of the installed websocketpp-config.cmake
       - *Or* set ``WEBSOCKETPP_INCLUDE_DIR`` (PATH) to the location of the WebSocket++ include files, e.g. *``<home-dir>``*``/cpprestsdk/Release/libs/websocketpp`` to use the copy within the C++ REST SDK source tree
3. Use CMake to generate build/project files, and then build
   "Visual Studio 14 2015 Win64" and more recent Visual Studio generators have been tested

**Windows**

For example, for Visual Studio 2017:
```
cd <home-dir>\nmos-cpp\Development
mkdir build
cd build
cmake .. ^
  -G "Visual Studio 15 2017 Win64" ^
  -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release"
```

<details>
<summary>Or if not using Conan...</summary>

```
cd <home-dir>\nmos-cpp\Development
mkdir build
cd build
cmake .. ^
  -G "Visual Studio 15 2017 Win64" ^
  -DCMAKE_CONFIGURATION_TYPES:STRING="Debug;Release" ^
  -DBoost_USE_STATIC_LIBS:BOOL="1" ^
  -DBOOST_INCLUDEDIR:PATH="<home-dir>/boost_1_75_0" ^
  -DBOOST_LIBRARYDIR:PATH="<home-dir>/boost_1_75_0/x64/lib" ^
  -DWEBSOCKETPP_INCLUDE_DIR:PATH="<home-dir>/cpprestsdk/Release/libs/websocketpp"
```

</details>

Then, open and build the generated nmos-cpp Visual Studio Solution.

Or on the Developer command line:
```
msbuild nmos-cpp.sln /p:Configuration=<Debug-or-Release>
```

**Linux**

For example, using the default toolchain and dependencies:

```
cd <home-dir>/nmos-cpp/Development
mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE:STRING="<Debug-or-Release>"
make
```

<details>
<summary>Or if not using Conan...</summary>

```
cd <home-dir>/nmos-cpp/Development
mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE:STRING="<Debug-or-Release>" \
  -DWEBSOCKETPP_INCLUDE_DIR:PATH="<home-dir>/cpprestsdk/Release/libs/websocketpp"
make
```

</details>

## Run Tests

Unit tests for parts of the nmos-cpp software are included in this repo.
The AMWA NMOS API Testing Tool may be used to test the REST APIs and behaviour of Node and Registry applications.

### Unit tests

The unit tests are currently packaged into a single test suite, as the **nmos-cpp-test** application.
This may be run automatically by building RUN_TESTS, but note that to see the output of any failed tests,
it is necessary to set ``CTEST_OUTPUT_ON_FAILURE`` to ``1`` in the environment first.

The build output should conclude something like this:

```
100% tests passed, 0 tests failed out of 52

Total Test time (real) =  18.50 sec
```

The application can also be run with other options, for example to run a single test case.
Use ``--help`` to display usage information.

Notes:
- On Windows, the correct configuration of the C++ REST SDK library (e.g. cpprestsdk140_2_10.dll or cpprest140_2_10d.dll) needs to be on the ``PATH`` or copied into the output directory.
- Intermittent failures of the DNS Service Discovery (DNS-SD) tests may be encountered because short time-outs and no retries are used in the test code.
  However if these tests fail repeatedly a system problem may need to be diagnosed.

### API tests

The [AMWA NMOS API Testing Tool](https://github.com/AMWA-TV/nmos-testing) is a Python 3 application that creates a simple web service which tests implementations of the NMOS APIs.

Having cloned the GitHub repository, install required dependencies with pip:

```
cd nmos-testing
python3 -m pip install -r requirements.txt
```

Then, launch the web service:

```
python3 nmos-test.py
```

It takes some time to start as it fetches the RAML and JSON Schema files from the AMWA NMOS specification repositories on GitHub.

Check the web service is running by opening http://localhost:5000/ in a browser.

There are several test suites for NMOS Nodes which can be run from the web service.

For example, to test **nmos-cpp-node**, try:

```
nmos-cpp-node "{\"http_port\":8080}"
```

Check it is running by opening http://localhost:8080/ in a browser.

Then run the "IS-04 Node API", "IS-05 Connection Management API" and "IS-05 Interaction with Node API" test suites from the web service.

There are also test suites for NMOS Registries.

For example, to test **nmos-cpp-registry**, try:

```
nmos-cpp-registry "{\"http_port\":1080}"
```

Check it is running by opening http://localhost:1080/ in a browser.

Then try running the "IS-04 Registry APIs" test suites from the web service.

## What Next?

The [tutorial](Tutorial.md) explains how to run the NMOS Registry and some example NMOS Nodes provided by nmos-cpp.
