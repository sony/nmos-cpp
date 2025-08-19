## Install nmos-cpp Using Conan

The following steps describe how to install and set up nmos-cpp using the Conan package manager.
For many platforms, a binary package is available from Conan Center Index so it isn't necessary to build nmos-cpp or any of its dependencies.

1. Install conan:
   ```sh
   pip install --upgrade conan~=2.0.5
   ```

   `pip` is the package installer for Python. Install Python 3 if necessary.

   If the python Scripts directory is not on the PATH you will get a warning like:
   > WARNING: The script conan.exe is installed in 'C:\Users\\%USERNAME%\\AppData\Local\Packages\PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0\LocalCache\local-packages\Python312\Scripts' which is not on PATH.
   > Consider adding this directory to PATH or, if you prefer to suppress this warning, use --no-warn-script-location.

   On Windows, you can use _System Properties \> Environment Variables..._ to permanently add the directory to the user PATH. Restart the Command Prompt and run `conan --help` to ensure Conan is found.

2. Detect profile:
   ```sh
   conan profile detect
   ```
   This will create a Conan profile with values based on your current platform, compiler, etc.
   Ideally the profile should be similar to these:
	
   Windows
   ```ini
   [settings]
   arch=x86_64
   build_type=Release
   compiler=msvc
   compiler.cppstd=14
   compiler.runtime=dynamic
   compiler.version=193
   os=Windows
   ```
   Linux
   ```ini
   [settings]
   arch=x86_64
   build_type=Release
   compiler=gcc
   compiler.cppstd=17
   compiler.version=11
   os=Linux
   ```

3. Install nmos-cpp:
   ```sh
   conan install --tool-requires=nmos-cpp/cci.20240223
   ```
   This installs the **nmos-cpp-registry** and **nmos-cpp-node** applications in the Conan cache, and generates a script to make these executables available in the current session.
   On Windows, run `.\conanbuild.bat` to add the install directory to the PATH.
   On Linux, run `source conanbuild.sh`.

   Alternatively the nmos-cpp installation can be copied to the current working directory using a Conan deployer:
   ```sh
   conan install --requires=nmos-cpp/cci.20240223 --deployer=direct_deploy
   ```

   On Windows, the executables are then found in the _.\direct_deploy\nmos-cpp\bin\Release_ directory.
   On Linux, the executables are found in the _./direct_deploy/nmos-cpp/bin_ directory.

4. Try starting nmos-cpp-registry and/or nmos-cpp-node:
   ```sh
   nmos-cpp-registry
   ```
   or
   ```sh
   nmos-cpp-node
   ```
   For more information about running these applications and the JSON configuration file that can be passed on the command-line, see the [tutorial](Tutorial.md).
