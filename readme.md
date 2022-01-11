[![Build status](https://ci.appveyor.com/api/projects/status/hxenwxmpaob5xung?svg=true)](https://ci.appveyor.com/project/ModOrganizer2/modorganizer-736bd)

# Here working on the Linux-native build of the Mod Organizer 2

This is work-in-progress project that aims to build native Linux binaries from the MO2 source code.

It's not usable in current state.

CMake scripts didn't require a lot of modifications.
Build instructions are here: [modorganizer-deb-packaging](https://github.com/ChipmunkV/modorganizer-deb-packaging) (scripts for generating MO2 deb package)

## Porting order and conventions

* comment-out all the code that doesn't compile, trace a `FIXME` message and call abort instead, mark temporary includes you need like that: `#include <cassert> // UNUSED`
* add missing includes, they are missing mostly due to the removal of the precompiled headers (PCH)
* fix cross-platform code
* launch and do stuff until an abort or a crash
* add back commented-out parts while adding Linux implementations for them, use `#ifdef _WIN32` where needed, keep `FIXME` for incomplete implementations

The commit structure is not defined, we'll look into organizing them before doing rebases.

For now building only the core libraries and plugins to get something workable and to see what's needed from the virtual filesystem.
Will probably reimplement the virtual filesystem by using FUSE or Linux OverlayFS.
Linux namespaces may be handy to make each game process see its own set of filesystem mounts.

Wine will be used for things like the Windows registry access.
Launching games from MO2 will probably rely on Lutris or Steam.

## Implementation details

### Mappings

Mappings is list of source-destination records that define which directories to lay over the other directories or (or override a single file by another file).
Its purpose is to define how to overlay multiple game mod directories over the original gamedata directory.

### Virtual filesystem using LD_PRELOAD

The LD_PRELOAD-based MO2 virtual filesystem is a standalone shared library that is not linked to anything.
It takes a list of mappings through an environment variable `VFS_MAPPING_LIST`.

MO2 when running the game process sets the `VFS_MAPPING_LIST` environment variable for the game process.
MO2 when running the game process sets the `LD_PRELOAD` environment variable for the game process to the path to the virtual filesystem shared library file.
The `LD_PRELOAD` variable is used by the `ld.so` dynamic linker in Linux.

The virtual filesystem shared library implements the logic of layering of multiple game mod directories on top of the game data directory according to the MO2 USVFS implementation.

The virtual filesystem shared library overrides the open, read, write, etc. functions of the libc to operate on files from a desired mapping.

The resulting combined directory appears at the place of the original gamedata directory according to the mappings.

### Virtual filesystem using FUSE

The FUSE-based MO2 virtual filesystem is a standalone executable.
It takes a list of mappings as command line arguments.

The executable mounts the mappings, they remain mounted while the executable is running.
For each directory or single-file mapping destination it creates a FUSE mount.

It implements the logic of layering of multiple game mod directories on top of the game data directory according to the MO2 USVFS implementation.

The virtual filesystem executable implements the open, read, write, etc. functions from `fuse_operations` to operate on files from a desired mapping.

The resulting combined directory appears at the place of the original gamedata directory according to the mappings.

In order to have access to the original gamedata directory after mounting the mappings the executable unmounts them just for itself.
To do that it first uses the `unshare` Linux syscall to enter a new Linux namespace, then the `umount` call to unmount the FUSE mount from the original gamedata directory.
It should not be confused with the other `unshare` call that is used by the game process runner to keep the mounts invisible to everything except the game process and its subprocesses.

The `unshare` call needs a higher level of access than an ordinary user has.
So the virtual filesystem executable file has the `cap_sys_admin+ep` attributes set just for that.
The executable sets up the mappings, `unshare`s the mount namespace, drops capabilities, `umount`s all the FUSE mounts, starts serving FUSE callbacks.

### Virtual filesystem using Linux overlayfs and binds

The overlayfs-based MO2 virtual filesystem is a standalone executable.
It takes a list of mappings as command line arguments.

The executable mounts the mappings and exits.

The "bind" mounts are used to override individual files.
Equivalent `mount` command to override `appdata/config.ini` by `config.ini` is:

`mount --bind config.ini appdata/config.ini`

The overlayfs mounts are used to overlay directories.
Equivalent `mount` command to overlay `mod1dir/`, `mod2dir/` and `mod3dir/` over `gamedata/` and use `Overwrite/` as the "MO2 Overwrite" dir is:

`mount -t overlay -o lowerdir=gamedata/:mod1dir/:mod2dir/:mod3dir/,upperdir=Overwrite/,workdir=/tmp/work overlay gamedata/`

The read-only lower dirs are the original gamedata dir and the mod dirs, the upper dir is the read-write "Overwrite" dir.
The resulting combined dir appears at the place of the original gamedata dir according to the mappings.

The behaviour is inconsistent with the MO2 USVFS implementation.

The `mount` call needs a higher level of access than an ordinary user has.
So the virtual filesystem executable file has the `cap_sys_admin+ep` attributes set.

### Game process runner for the per-process visibility of the mount-based virtual filesystem

The game process runner is an executable that takes a path to the virtual filesystem executable and a list of mappings as command line arguments.

To make the mount operations affect only the process of the game (and its subprocesses) the game process runner prepares a "Linux mount namespace" for it before starting the game.
It uses the `unshare` Linux syscall.
Equivalent command:

`unshare --mount`

The mounts set up by this process will be visible only to this process and its children.

The `unshare` call needs a higher level of access than an ordinary user has.
So the runner executable file has the `cap_sys_admin+ep` attributes set.

The runner performs `unshare`, drops capabilities, executes the virtual filesystem executable and runs the game.

### Virtual filesystem library

The virtual filesystem library consists of the library itself and a C++ header with the API.
It is linked to a program that wants to define mappings.

For MO2 the virtual filesystem library is built into a Qt plugin.

### MO2 Windows-specific tools

* `winapi-PrivateProfile.sh` - command line tool to read and write INI files using WinAPI

Compiled at runtime.

## Motivation

* Linux users are more likely to look into the code and fix the issues if they don't need a Windows VM to build and debug MO2
* Linux users won't need a deep knowledge of Wine when debugging issues with MO2
* game runtime performance and battery life should be better with Linux OverlayFS

# Mod Organizer

Mod Organizer (MO) is a tool for managing mod collections of arbitrary size. It is specifically designed for people who like to experiment with mods and thus need an easy and reliable way to install and uninstall them.
Mod Organizer 2 is a mod manager created by Tannin to support 64-bit games like Skyrim Special Edition and Fallout 4 in addition to all the 32-bit games MO1 already supported.

Tannin discontinued the project when he was hired by the Nexus team to develop their new Vortex mod manager.
MO2 was not completed and was left with many issues. LePresidente managed to fix it to a usable state by himself after SkyrimSE came out, adding support for it, and releasing v2.0.8.
The project took up speed again after a few more coders showed up in late 2017, and a lot of progress has been made.

## Help Wanted!
Mod Organizer 2 is an open project in the hands of the community, there are problems that need to be solved and things that could be added. MO2 really needs developers and if you have the programming skills and some free time you can really improve the experience of the modding community.

To have more information, please join the open MO2 Development discord server: [Mod Organizer 2](https://discord.gg/6GKR9jZ)
If you want to help translate MO2 to your language you should join the discord server too and head to the #translation channel.
To setup a development environment on your machine, there is the [mob project](https://github.com/modorganizer2/mob) that handles that.
If you want to submit your code changes, please use a good formatting style like the default one in Visual Studio.

Through the work of a few people of the community MO2 has come quite far, now it needs some more of those people to go further.

## Reporting Issues:
Issues should be reported to the GitHub page or on the open discord server: [Mod Organizer 2](https://discord.gg/6GKR9jZ). Here is also where dev builds are tested, bugs are reported and investigated, suggestions are discussed and a lot more.

Credits to Tannin, LePresidente, Silarn, erasmux, AL12, LostDragonist, AnyOldName3, isa, Holt59 and many others for the development.

## Download Location

* on [GitHub.com](https://github.com/Modorganizer2/modorganizer/releases)
* on [NexusMods.com](https://www.nexusmods.com/skyrimspecialedition/mods/6194)

## Old Download Location

* [TanninOne/modorganizer/releases](https://github.com/TanninOne/modorganizer/releases)

## Building

Please refer to [Modorganizer2/mob](https://github.com/modorganizer2/mob) for build instructions.

## Other Repositories

MO2 consists of multiple repositories on github. The mob project will download them automatically as required. They should however also be buildable individually.
Here is a complete list:

* https://github.com/LePresidente/cpython-1
* https://github.com/ModOrganizer2/cmake_common
* https://github.com/ModOrganizer2/githubpp
* https://github.com/ModOrganizer2/modorganizer
* https://github.com/ModOrganizer2/modorganizer-archive
* https://github.com/ModOrganizer2/modorganizer-basic_games
* https://github.com/ModOrganizer2/modorganizer-bsatk
* https://github.com/ModOrganizer2/modorganizer-bsa_extractor
* https://github.com/ModOrganizer2/modorganizer-check_fnis
* https://github.com/ModOrganizer2/modorganizer-diagnose_basic
* https://github.com/ModOrganizer2/modorganizer-esptk
* https://github.com/ModOrganizer2/modorganizer-fnistool
* https://github.com/ModOrganizer2/modorganizer-helper
* https://github.com/ModOrganizer2/modorganizer-game_enderal
* https://github.com/ModOrganizer2/modorganizer-game_fallout3
* https://github.com/ModOrganizer2/modorganizer-game_fallout4
* https://github.com/ModOrganizer2/modorganizer-game_fallout4vr
* https://github.com/ModOrganizer2/modorganizer-game_falloutnv
* https://github.com/ModOrganizer2/modorganizer-game_features
* https://github.com/ModOrganizer2/modorganizer-game_gamebryo
* https://github.com/ModOrganizer2/modorganizer-game_morrowind
* https://github.com/ModOrganizer2/modorganizer-game_oblivion
* https://github.com/ModOrganizer2/modorganizer-game_skyrim
* https://github.com/ModOrganizer2/modorganizer-game_skyrimSE
* https://github.com/ModOrganizer2/modorganizer-game_skyrimVR
* https://github.com/ModOrganizer2/modorganizer-game_ttw
* https://github.com/ModOrganizer2/modorganizer-installer_bain
* https://github.com/ModOrganizer2/modorganizer-installer_wizard
* https://github.com/ModOrganizer2/modorganizer-installer_bundle
* https://github.com/ModOrganizer2/modorganizer-installer_fomod
* https://github.com/ModOrganizer2/modorganizer-installer_fomod_csharp
* https://github.com/ModOrganizer2/modorganizer-installer_manual
* https://github.com/ModOrganizer2/modorganizer-installer_ncc
* https://github.com/ModOrganizer2/modorganizer-installer_omod
* https://github.com/ModOrganizer2/modorganizer-installer_quick
* https://github.com/ModOrganizer2/modorganizer-lootcli
* https://github.com/ModOrganizer2/modorganizer-NCC
* https://github.com/ModOrganizer2/modorganizer-nxmhandler
* https://github.com/ModOrganizer2/modorganizer-plugin_python
* https://github.com/ModOrganizer2/modorganizer-preview_base
* https://github.com/ModOrganizer2/modorganizer-preview_dds
* https://github.com/ModOrganizer2/modorganizer-form43_checker
* https://github.com/ModOrganizer2/modorganizer-script_extender_plugin_checker
* https://github.com/ModOrganizer2/modorganizer-tool_configurator
* https://github.com/ModOrganizer2/modorganizer-tool_inibakery
* https://github.com/ModOrganizer2/modorganizer-uibase
* https://github.com/ModOrganizer2/usvfs

### Unused Repositories
* https://github.com/ModOrganizer2/modorganizer-hookdll
* https://github.com/TanninOne/modorganizer-tool_nmmimport
