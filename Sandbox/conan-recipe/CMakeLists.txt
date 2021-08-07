# see https://github.com/conan-io/cmake-conan/issues/249#issuecomment-737011732
cmake_minimum_required(VERSION 3.17)
project(cmake_wrapper)

include(conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_subdirectory("nmos-cpp/Development")
