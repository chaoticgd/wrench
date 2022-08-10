# Wrench Editor

A set of modding tools for the Ratchet & Clank PS2 games. Works with R&C1, R&C2, R&C3 and Deadlocked. Work in progress.

To download the latest Windows build, see the [Releases](https://github.com/chaoticgd/wrench/releases) page. Linux users can follow the build instructions below.

Features currently include:
- **New in v0.1!** Asset System
	- A system to create, distribute and load mods.
	- Multiple mods can be loaded at a time.
- **New in v0.1!** Launcher
	- A user interface to manage mods.
	- Used to launch the level editor.
- Build Tool
	- Pack/unpack entire ISO files.
	- Unpack moby meshes as COLLADA files.
	- Pack/unpack textures as PNG files.
	- Pack/unpack collision meshes as COLLADA files.
	- **New in v0.2!** Pack/unpack sky meshes as COLLADA files.
	- Pack/unpack everything else as binary files.
- Level Editor
	- View unpacked levels.
	- Move objects and modify their attributes.

For documentation on the asset system, see [Asset System](docs/asset_system.md) and [Asset Reference](docs/asset_reference.md). For information on the game's file formats, see the [Formats Guide](docs/formats_guide.md).

## Screenshots

![Launcher](docs/screenshots/launcher.png)
![Level Editor](docs/screenshots/editor.png)

## Building

### Linux

1.	Install the following dependencies and tools:
	- git
	- cmake
	- g++ 8 or newer
	- xorg-dev (needed to build GLFW)
	- zenity

2.	cd into the directory above where you want Wrench to live e.g. `cd ~/code`.

2.	Download the source code and additional dependencies using Git:
	> git clone --recursive https://github.com/chaoticgd/wrench

3.	cd into the newly created directory:
	> cd wrench

4.	Build it with cmake:
	> cmake . && cmake --build . -- -j8
	
	(in the above example 8 threads are used)

### Windows

1.	Install the following tools:
	- git
	- Visual Studio (with desktop C++/cmake support)

2.	Open a Visual Studio developer command prompt.

3.	cd into the directory above where you want Wrench to live e.g. `cd c:\code`.

4.	Download the source code and dependencies using Git:
	> git clone --recursive https://github.com/chaoticgd/wrench

5.	cd into the newly created directory:
	> cd wrench

6.	Generate cmake files:
	> cmake .

	This should generate `wrench.sln` along with a few `.vcxproj` files. 
	In case no such files are generated, you can explicitly specify usage of the Visual Studio generator by running the following command:
	> cmake . -G "Visual Studio X YYYY"
	where `X` is the Visual Studio version and `YYYY` is the Visual Studio year (example: `Visual Studio 16 2019`)
	A complete list can be obtained by running `cmake --help`.

7.	Build the project
	* From the command line

	> cmake --build . --config BUILD_TYPE

	where `BUILD_TYPE` is one of `Debug` (very slow - not recommended), `Release` (no symbols - not recommended), `RelWithDebInfo` (recommended) or `MinSizeRel`.

	* From Visual Studio
	
	Open the newly generated `wrench.sln` in Visual Studio. In the Solution Explorer, right-click on `wrench` and click `Set as Startup Project`.
	You should now be able to build and debug wrench using the toolbar controls and all Visual Studio features.
