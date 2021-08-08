cmake_minimum_required(VERSION 3.1)
project(NmosCppPackageTest CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(nmos-cpp REQUIRED)

add_executable(nmos-cpp-package-test nmos-cpp-package-test.cpp)
target_link_libraries(nmos-cpp-package-test nmos-cpp::compile-settings nmos-cpp::nmos-cpp)
