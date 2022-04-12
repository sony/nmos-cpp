# Cross-compiling for Raspberry Pi

The following instructions have been used to cross-compile nmos-cpp and all its dependencies for the Raspberry Pi, from a Linux (Ubuntu 14.04 LTS) box.

## Set up Raspberry Pi Tools

First, create a directory to install the  [Raspberry Pi Tools](https://github.com/raspberrypi/tools) and required components.
For the rest of these instructions, the directory ``~/rpi`` is assumed.

```sh
mkdir ~/rpi
cd ~/rpi

# Get the Raspberry Pi Tools.
git clone https://github.com/raspberrypi/tools.git

# Set up environment variables for the Raspberry Pi Tools.
cd ~/rpi
export RPI_ROOT=$(pwd)
export RPI_LIBS=${RPI_ROOT}/libs
export RPI_TOOLS_DIR=${RPI_ROOT}/tools
export PATH=$PATH:${RPI_ROOT}/tools/arm-bcm2708/arm-linux-gnueabihf/bin

# Set up environment variable for package configuration
export PKG_CONFIG_PATH=${RPI_ROOT}/lib/pkgconfig

# Set up environment variables for cross-compiling those dependencies not using CMake.
export CROSS=arm-linux-gnueabihf
export CC=${CROSS}-gcc
export LD=${CROSS}-ld
export AS=${CROSS}-as
export AR=${CROSS}-ar
```

An [example CMake toolchain file](../Development/rpi/Toolchain-rpi.cmake) is provided in this repo.
Copy it to ``~/rpi``.

## Dependencies

As described in the main [Dependencies](Dependencies.md) notes, the nmos-cpp codebase utilizes a number of other open-source projects.
In addition to the Boost C++ Libraries, C++ REST SDK which need to be built for most platforms, when cross-compiling for Raspberry Pi, a number of other libraries probably need to be built.
They are OpenSSL, Avahi, and its dependencies (Expat, libdaemon and D-Bus).
All cross-compiled components will be stored in the directory specified by ``${RPI_LIBS}``, e.g. ``~/rpi/libs``.

### OpenSSL

```sh
# Fetch the library.
cd ~
git clone https://github.com/openssl/openssl.git

# Configure for cross-compiling.
cd openssl
./Configure linux-generic32 --prefix=${RPI_LIBS} --openssldir=${RPI_LIBS}/openssl

# Cross-compile and install into ${RPI_LIBS}.
make
make install
```

### Boost C++ Libraries

```sh
# Download and unpack the libraries.
cd ~
wget https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.tar.gz 
tar -xf boost_1_78_0.tar.gz

# Prepare Boost tools.
cd boost_1_78_0
./bootstrap.sh

# Configure for cross-compiling.
echo "import option ; import feature ; using gcc : arm : arm-linux-gnueabihf-g++ ;" > ~/user-config.jam

# Cross-compile required Boost libraries, and install into ${RPI_LIBS}.
./b2 install toolset=gcc-arm \
  --prefix=${RPI_LIBS} \
  --with-atomic \
  --with-chrono \
  --with-date_time \
  --with-filesystem \
  --with-random \
  --with-regex \
  --with-system \
  --with-thread \
  --stagedir=. stage
```

### C++ REST SDK

```sh
# Fetch the library.
cd ~
git clone --recurse-submodules https://github.com/Microsoft/cpprestsdk.git

# Configure for cross-compiling.
cd cpprestsdk/Release
mkdir build.rpi
cd build.rpi
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCPPREST_EXCLUDE_COMPRESSION=1 \
  -DWERROR=0 \
  -DCMAKE_TOOLCHAIN_FILE=${RPI_ROOT}/Toolchain-rpi.cmake \
  -DCMAKE_INSTALL_PREFIX=${RPI_LIBS} \
  -DBUILD_SAMPLES=0 \
  -DBUILD_TESTS=0

# Cross-compile and install into ${RPI_LIBS}.
make
make install
```

### Expat

This XML parser library is required by Avahi.

```sh
# Download and unpack the library.
cd ~
wget https://github.com/libexpat/libexpat/releases/download/R_2_2_5/expat-2.2.5.tar.bz2
tar -xf expat-2.2.5.tar.bz2

# Configure for cross-compiling.
cd expat-2.2.5
./configure --host=arm-linux-gnueabihf --prefix=${RPI_LIBS} ac_cv_func_setpgrp_void=yes 

# Cross-compile and install into ${RPI_LIBS}.
make
make install
```

### libdaemon

This library for writing daemons is required by Avahi.

```sh
# Download and unpack the library.
cd ~
wget http://0pointer.de/lennart/projects/libdaemon/libdaemon-0.14.tar.gz
tar -xf libdaemon-0.14.tar.gz

# Configure for cross-compiling.
cd libdaemon-0.14
./configure --host=arm-linux-gnueabihf --prefix=${RPI_LIBS} ac_cv_func_setpgrp_void=yes

# Cross-compile and install into ${RPI_LIBS}.
make
make install
```

### D-Bus

This library for interprocess communication is required by Avahi.

```sh
# Download and unpack the library.
cd ~
wget https://dbus.freedesktop.org/releases/dbus/dbus-1.12.8.tar.gz
tar -xf dbus-1.12.8.tar.gz

# Configure for cross-compiling.
cd dbus-1.12.8
./configure --host=arm-linux-gnueabihf --prefix=${RPI_LIBS} \
  CPPFLAGS=-I${RPI_LIBS}/include \
  LDFLAGS=-L${RPI_LIBS}/lib \
  EXPAT_CFLAGS=" " \
  EXPAT_LIBS=-lexpat

# Cross-compile and install into ${RPI_LIBS}.
make
make install
```

### Avahi

Although Avahi is pre-installed on the Raspberry Pi, the optional *avahi-compat-libdns_sd* library must be built.

```sh
# Download and unpack the library.
cd ~
wget https://github.com/lathiat/avahi/releases/download/v0.7/avahi-0.7.tar.gz
tar -xf avahi-0.7.tar.gz

# Configure for cross-compiling.
cd avahi-0.7
./configure --host=arm-linux-gnueabihf --prefix=${RPI_LIBS} \
   CPPFLAGS=-I${RPI_LIBS}/include \
   LDFLAGS=-L${RPI_LIBS}/lib \
   --with-distro=none \
   --disable-glib --disable-gobject --disable-gdbm --disable-qt3 --disable-qt4 --disable-gtk --disable-gtk3 --disable-mono --disable-monodoc --disable-python \
   --enable-compat-libdns_sd \
   --with-systemdsystemunitdir=no \
   LIBDAEMON_CFLAGS=" " \
   LIBDAEMON_LIBS=-ldaemon

# Cross-compile and install into ${RPI_LIBS}.
make
make install
```

## Building nmos-cpp

For more detailed instructions about building nmos-cpp itself, see the main [Getting Started](Getting-Started.md) instructions.

```sh
# Fetch the library.
cd ~
git clone https://github.com/sony/nmos-cpp.git

# Configure for cross-compiling.
cd ~/nmos-cpp/Development
mkdir build.rpi
cd build.rpi
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=${RPI_ROOT}/Toolchain-rpi.cmake \
  -DWEBSOCKETPP_INCLUDE_DIR:PATH="~/cpprestsdk/Release/libs/websocketpp" \
  -DNMOS_CPP_USE_AVAHI=1 \
  -DAvahi_INCLUDE_DIR=${RPI_LIBS}/include/avahi-compat-libdns_sd \
  -DNMOS_CPP_USE_CONAN=0

# Cross-compile the library.
make
```

Note that the following error may be reported after linking nmos-cpp-test.

```
nmos-cpp-test: Syntax error: word unexpected (expecting ")")
```

It is because the dynamic test discovery attempts to run the cross-compiled nmos-cpp-test, which of course doesn't work.

## Prepare the Raspberry Pi

1. On the Raspberry Pi, prepare directories for the software
     ```sh
     cd ~
     mkdir bin
     mkdir lib
     ```
2. Enable SSH
   * Choose menu Preferences > Raspberry Pi Configuration
   * Under the Interfaces tab, select the "SSH: Enabled" option
   * Click OK to reboot the Pi  
     Note: Default username: pi, password: raspberry
3. Set a static IP address for the Pi
   ```sh
   sudo nano /etc/dhcpcd.conf
   ```
   * Uncomment the following lines under "Example static IP configuration"
     ```
     static ip_address=192.168.0.10/24 
     static routers=192.168.0.1 
     static domain_name_servers=192.168.0.1 8.8.8.8
     ```
   * Uncomment the following line under "fallback to static profile on eth0"
     ```
     interface eth0
     ```
   ```sh
   sudo reboot
   ```
   The Pi is now configured with 192.168.0.10 as its static IP address.
4. Set up the host Linux machine's network interface
   * Set an appropriate mask and address, e.g. 192.168.0.20, to be able to communicate with the Pi
   * Confirm the Pi responds to a ``ping`` via this interface
5. Transfer nmos-cpp and dependences to the Pi with ``scp``
   ```sh
   cd nmos-cpp/Development/build.rpi
   scp nmos-cpp-registry nmos-cpp-node pi@192.168.0.10:/home/pi/bin
   scp ~/rpi/libs/lib/* pi@192.168.0.10:/home/pi/lib
   ```

## Start an NMOS Registry and Node on the Raspberry Pi

Now try running the NMOS Registry and an NMOS Node provided by nmos-cpp. See the [Tutorial](Tutorial.md) for more information.

On the Raspberry Pi, open a terminal:

* Ensure avahi-daemon is running, ``ps -e | grep avahi-daemon``; if not running, do ``sudo avahi-daemon &``
* Start the NMOS Registry
  ```sh
  cd ~/bin
  export LD_LIBRARY_PATH=~/lib
  ./nmos-cpp-registry "{\"http_port\":8080}"
  ```

Open a second terminal to run an NMOS Node.

* Start the NMOS Node
  ```sh
  cd ~/bin
  export LD_LIBRARY_PATH=~/lib
  ./nmos-cpp-node "{\"http_port\":1080}"
  ```

The Node should register successfully with the Registry.
