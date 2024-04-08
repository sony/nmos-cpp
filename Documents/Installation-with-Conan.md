## Install and Set Up nmos-cpp Using Conan

The following steps describe how to install and set up nmos-cpp using the Conan package manager.
For many platforms, a binary package is available from Conan Center Index so it isn't necessary to build nmos-cpp or any of its dependencies.

1. Install conan:
   ```sh
   pip install --upgrade conan~=2.0.5
   ```

   If the python Scripts directory is not on the PATH you will get a warning like:
   > WARNING: The script conan.exe is installed in 'C:\Users\%USER%\AppData\Local\Packages\PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0\LocalCache\local-packages\Python312\Scripts' which is not on PATH.
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
   Make the `nmos-cpp-registry` and `nmos-cpp-node` executables available in the current session.
   On Windows, run `.\conanbuild.bat`.
   On Linux, run `./conanbuild.sh`.

   Alternatively if the nmos-cpp-node and nmos-cpp-registry has still not been added to path, you can add them manually (Executables likely located at C:\Users\%USERNAME%\.conan2\p\nmos-768c7905bf562\p\bin\Release) or work around this as shown below:

    
	> Executables can be copied to current working directory using a direct installation:\
	> `conan install --requires=nmos-cpp/cci.20240223 --deployer=direct_deploy`
   	> - `--deployer=direct_deploy` copies the direct dependencies, in this case nmos-cpp-registry.exe and nmos-cpp-node.exe.
   	>
 	> \
	> Launch the registry using `./direct_deploy/nmos-cpp/bin/Release/nmos-cpp-registry.exe <your_config.json>` \
	> Launch nodes using `./direct_deploy/nmos-cpp/bin/Release/nmos-cpp-node.exe <your_config.json>`.
  
4. Try running nmos-cpp-registry and/or nmos-cpp-node:
   If your executables were added to the PATH you could call then directly like:  `nmos-cpp-node <your_config.json>` and `nmos-cpp-registry <your_config.json>`
