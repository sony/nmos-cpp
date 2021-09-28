# this one is important
SET (CMAKE_SYSTEM_NAME Linux)
# this one not so much
SET (CMAKE_SYSTEM_VERSION 1)

# check for Raspberry Pi Tools
if(DEFINED ENV{RPI_TOOLS_DIR})
  message(STATUS "Using Raspberry Pi Tools directory at " $ENV{RPI_TOOLS_DIR})
else()
  message(FATAL_ERROR "Please set RPI_TOOLS_DIR to the Raspberry Pi Tools directory (https://github.com/raspberrypi/tools)")
endif()

SET (RPI_TOOLCHAIN arm-bcm2708/arm-linux-gnueabihf)

# specify the cross compiler
SET (CMAKE_C_COMPILER $ENV{RPI_TOOLS_DIR}/${RPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-gcc)
SET (CMAKE_CXX_COMPILER $ENV{RPI_TOOLS_DIR}/${RPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-g++)

# where is the target environment
SET (CMAKE_FIND_ROOT_PATH $ENV{RPI_TOOLS_DIR}/${RPI_TOOLCHAIN}/arm-linux-gnueabihf/sysroot/ $ENV{RPI_LIBS})

# search for programs in the build host directories
SET (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
