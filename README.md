# LegacyClonk

![Logo](planet/Graphics.c4g/Logo.png)

LC (short for LegacyClonk) is a fan project based on Clonk Rage.
LegacyClonk continues to receive updates and ensures compatibility with existing Clonk Rage content.

The goal is to fix as many bugs and inelegances as possible, improving quality-of-life and adding nice-to-have features while not impacting the gameplay we’re all used to.

## Installation
Please refer to the [English](https://clonkspot.org/lc-en#installation-1) or [German](https://clonkspot.org/lc#installation-1) installation manual if you simply want to play the game.

[clonkspot.org](https://clonkspot.org) hosts the community forum, content developer documentation and masterserver.

## Compiling - Quick Start
**Disclaimer**: This readme is fairly new and may contain mistakes. In case of problems [reach out](#Contact). Readme improvement PRs are welcome.

Essential dependencies:
- CMake
- Fairly modern C++ compiler
	- Windows: **Latest MSVC**
	- Linux: **g++ ≥ 15.1** or **clang++ ≥ 20.1**
	- macOS: **open source clang++ ≥ 20.1** - e.g. `brew install llvm@20 ninja`
- Make or Ninja

Extract the [latest pre-built dependencies](https://github.com/legacyclonk/deps/releases/latest) for your platform into a folder called `deps`.
Make sure that besides the `CMakeLists.txt` of LegacyClonk there are folders `deps/include`, `deps/lib`, etc. as extracted from the binary package.


### Configuring and compiling the engine
Open a Terminal and navigate to the repository root, such that `CMakeLists.txt` is in the current directory.

#### Windows
Open the CMake project using Visual Studio.

#### Linux
Configure with CMake
```bash
cmake . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_DEVELOPER_MODE=ON
```

Compile with CMake
```bash
cmake --build build
```

#### macOS
Configure with CMake
```bash
cmake . -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/autobuild/clang_mac.cmake
```

Compile with CMake
```bash
cmake --build build
```

Create the `.app`-bundle
```bash
DESTDIR=build/destdir cmake --build build --target install
```

The `clonk.app` should be generated at `build/destdir/usr/local/clonk.app`.


### Setup game folder
The engine needs various files to run placed next to the engine binary.
The cleanest way is to create and setup a game folder somewhere outside of the source code repository.

If you want to use an existing game folder (e.g. obtained via the simple binary [Installation](#Installation)) use that folder instead and only do step 2.

1. Create an empty folder at the desired location.
The guide assumes the directory path to be `~/clonk`.
1. Create a symbolic link to the binary in the folder
	- Windows: `New-Item -ItemType Symlink -Path ~/clonk/MyClonk.exe -Target build/clonk.exe` (needs to be run in a terminal with administrator rights)
	- Linux: `ln -s build/clonk ~/clonk/myclonk`
	- macOS: `ln -s build/clonk.app ~/clonk/myclonk.app`
1. Create symbolic links to [Graphics.c4g](planet/Graphics.c4g) and [System.c4g](planet/System.c4g). Without them, the engine won’t start.
	- Windows
	```PowerShell
	New-Item -ItemType Junction -Path ~/clonk/Graphics.c4g -Target planet/Graphics.c4g
	New-Item -ItemType Junction -Path ~/clonk/System.c4g -Target planet/System.c4g
	```

	- Linux / macOS
	```bash
	ln -s planet/Graphics.c4g planet/System.c4g ~/clonk
	```
1. Get official content (optional, but can’t play without some content)
	- Pre-built: Extract `lc_content.zip` from the [latest release](https://github.com/legacyclonk/content/releases/latest) in the game folder.
	- From repository
		1. Clone the content repository in a different folder, e.g. `~/lc_content`: `git clone https://github.com/legacyclonk/content ~/lc_content`
		2. Create symbolic links for all groups
			- Windows
				- Easier: Copy all directories with ending `.c4?`
				- When planning to edit the content: https://superuser.com/a/1489528
			- Linux / macOS: `ln -s ~/lc_content/*.c4? ~/clonk`

### Play

- Enter your game folder, e.g. `~/clonk`.
- Launch `clonk.exe`/`clonk.app`/the `clonk` binary.
- Enjoy

Remark: In order to join network games with other players (who use release binaries) your `System.c4g` needs to match the one from the host. It can be obtained from [Releases](https://github.com/legacyclonk/LegacyClonk/releases/latest).

## Contact

- IRC: [irc://irc.euirc.net/#legacyclonk](irc://irc.euirc.net/#legacyclonk)
- Discord: [Server](https://discord.gg/km58ETK)
- Forum: [https://forum.clonkspot.org](https://forum.clonkspot.org)
