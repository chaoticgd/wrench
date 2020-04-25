# wrench

A set of modding tools for the Ratchet & Clank PS2 games.

## Screenshots

![Level editor](screenshots/editor.png)
![Texture browser](screenshots/texture-browser.png)

## Features

- View levels from Ratchet & Clank 2 and 3.
- Store mods as .wrench files that double as project files and compression-aware patch files.
- Extract and replace certain textures.
- Extract racpak (*.WAD) archives.
- Decompress and recompress WAD segments (not to be confused with the *.WAD files on the game's filesystem).
- A number of command-line tools for testing.

## Compatibility

This project is probably not very usable yet, however the following game
releases are currently somewhat supported:

| ELF Identifier | Title                              | Edition     | Region | Status                     |
|----------------|------------------------------------|-------------|--------|----------------------------|
| SCES_516.07    | Ratchet & Clank: Locked and Loaded | Black Label | Europe | Skins, HUD images, levels. |
| SCES_524.56    | Ratchet & Clank 3                  | Black Label | Europe | Skins, levels.             |
| SCES_532.85    | Ratchet: Gladiator                 | Black Label | Europe | Skins only.                |

## Building

### Linux

1.	Install the following dependencies and tools:
	- git
	- cmake
	- g++ 8 or newer
	- libxinerama-dev and libxcursor-dev (needed to build GLFW)
	
	Ubuntu 18.04: You may need a newer version cmake than is available in the official repositories.

2.	cd into the directory above where you want Wrench to live e.g. `cd ~/code`.

2.	Download the source code and additional dependencies using Git:
	> git clone --recursive https://github.com/chaoticgd/wrench

3.	cd into the newly created directory:
	> cd wrench

4.	Build it with cmake:
	> cmake . && cmake --build .
	
	On Ubuntu 18.04, to use g++ 8 (from package `g++-8`) instead of the default compiler, you can instead run:
	> cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-8 && cmake --build .

### Windows

1.	Install the following tools:
	- git
	- Visual Studio (with cmake integration)

2.	Open a Visual Studio developer command prompt.

3.	cd into the directory above where you want Wrench to live e.g. `cd c:\code`.

4.	Download the source code and dependencies using Git:
	> git clone --recursive https://github.com/chaoticgd/wrench

5.	cd into the newly created directory:
	> cd wrench

6.	Build it with cmake:
	> cmake . && cmake --build .
