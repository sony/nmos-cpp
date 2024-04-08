## Guide to Install and Setup nmos-cpp using Conan Package Manager: 

1. Install conan:
`pip install --upgrade conan>=2.0.5`

    If conan.exe is not in PATH you will get warning: 
	 

	> WARNING: The script conan.exe is installed in 'C:\Users\%USER%\AppData\Local\Packages\PythonSoftwareFoundation.Python.3.12_qbz5n2kfra8p0\LocalCache\local-packages\Python312\Scripts' which is not on PATH.
	  Consider adding this directory to PATH or, if you prefer to suppress this warning, use --no-warn-script-location.


   Then the shown path needs to be added manually. After maunally adding the PATH, Restart the command line and run `conan --help` to ensure Conan is added to path successfully.

3. Detect Profile:
	Run `conan profile detect`. \
  This will create a conan profile with values based on your dev setup. Ideally the profile should be similar to these:
	
   For windows:
	```{txt}
	[settings]
	arch=x86_64
	build_type=Release
	compiler=msvc
	compiler.cppstd=14
	compiler.runtime=dynamic
	compiler.runtime_type=Release
	compiler.version=193
	os=Windows
	```
	For Linux:
	```{txt}
	[settings]
	arch=x86_64
	build_type=Release
	compiler=gcc
	compiler.cppstd=17
	compiler.version=11
	os=Linux
	```

4. Install nmos cpp:
	`conan install --tool-requires=nmos-cpp/cci.20240223`\
  If on windows, also run `./conanbuild.bat` to ensure the executables for nmos-cpp-node and nmos-cpp-registry are added to path.

   Alternatively if the nmos-cpp-node and nmos-cpp-registry has still not been added to path, you can add them manually (Executables likely located at C:\Users\%USERNAME%\.conan2\p\nmos-768c7905bf562\p\bin\Release) or work around this as shown below:

    
	> Executables can be copied to current working directory using a direct installation:\
	> `conan install --requires=nmos-cpp/cci.20240223 --deployer=direct_deploy`
   	> - `--deployer=direct_deploy` copies the direct dependencies, in this case nmos-cpp-registry.exe and nmos-cpp-node.exe.
   	>
 	> \
	> Launch the registry using `./direct_deploy/nmos-cpp/bin/Release/nmos-cpp-registry.exe <your_config.json>` \
	> Launch nodes using `./direct_deploy/nmos-cpp/bin/Release/nmos-cpp-node.exe <your_config.json>`.
  
  If your executables where added to PATH you could call then directly like:  `nmos-cpp-node <your_config.json>` and `nmos-cpp-registry <your_config.json>`
