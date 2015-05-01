#op64

op64 is an experimental 64-bit emulator for Windows and Linux (future), based on the [Project 64](http://www.pj64-emu.com/) and [mupen64plus](https://code.google.com/p/mupen64plus/) cores, with support for the zilmar plugin specification plugins. It is licensed under the GNU General Public License version 2.

## System Requirements
* OS
    * Microsoft Windows (Vista or higher).
    * Linux (future).
* CPU
    * A 64-bit CPU with SSE2 support (all known 64-bit desktop CPUs have SSE2 support)

## Build Dependencies##

These dependencies are large in size and thus cannot be included with the repository and must be manually downloaded.

* (Required) [Boost Libraries](http://www.boost.org/)
    * op64 uses the Boost library for cross platform abstractions that are not available in the C++ Standard Library
    * Precompiled .lib's for Visual Studio 2013 and Intel C++ 15.0 for the components used in op64 are available within the repository
    * Set the environment variable `BOOST_ROOT` to the root of the Boost library installation
* (Required) [Qt 5.3 or higher](http://www.qt.io/) for the Qt GUI project
    * Install an opensource 64-bit build for your OS and compiler
    * Alternatively, Windows Qt compact builds without the icu and webkit modules are available at http://sourceforge.net/projects/qtx64/. These builds remove the dependency of the gigantic icu dlls
    * Alternatively, acquire the source code and build Qt with the modules that you want. This option also allows you to build statically linked versions of the Qt libraries which removes the dependency of the gigantic Qt redistributable dlls (official Qt builds are dynamically linked, meaning Qt dlls must be packaged with the application)
    * Set the environment variable `QTDIR` to the root of your Qt installation (contains the include and lib directories)
    * Qt Visual Studio Add-in is recommended for Visual Studio users
* op64 supports some minor optimizations via Cilk Plus where available:
    * Intel C++
    * GCC 4.9 or later with the `-fcilkplus` flag
    * [llvm-clang cilkplus branch](https://cilkplus.github.io/) with the `-fcilkplus` flag

## Building on Windows

op64 uses C++11 features that are not available in compilers older than Visual Studio 2013. However, Visual Studio 2013 itself has several problems with standard C++11 features. Ugly workarounds are currently in place to work around these issues for Visual Studio 2013 and will be removed when the issues are fixed in a later update or version. Visual Studio 2015  fixes these issues. Intel C++ 15.0 works perfectly as it fully supports C++11.

### Compilers Tested

* Visual Studio 2013
* Intel C++ 15.0

### Building

Use the `op64.sln` solution file and compile the op64-qt project

## Building on Linux

Coming in the future

## Plugins

Since the application is 64-bit, plugins must also be compiled in 64-bit to work. This means all legacy 32-bit plugins do not work with op64. This is a hard restriction and there are no workarounds for this.

Plugins based on the zilmar plugin specification are supported. Support for the mupen64plus plugin specification is planned.

### Folder Structure

op64 uses the same folder structure as Project 64, with a top level `Plugins/` directory and 4 subdirectories `Audio/` `GFX/` `Input/` and `RSP/`. Plugins of each type should be placed in the appropriate directory in order for them to be detected.