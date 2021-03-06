#op64

op64 is an experimental 64-bit emulator for Windows and Linux, based on the [Project 64](http://www.pj64-emu.com/) and [mupen64plus](https://code.google.com/p/mupen64plus/) cores, with support for the zilmar plugin specification plugins. It is licensed under the GNU General Public License version 2.

## System Requirements
* OS
    * Microsoft Windows (Vista or higher).
    * Linux (experimental).
* CPU
    * A 64-bit CPU with SSE2 support (all known 64-bit desktop CPUs have SSE2 support)

## Build Dependencies##

These dependencies are large in size and thus cannot be included with the repository and must be manually downloaded.

* [Boost Libraries](http://www.boost.org/)
    * op64 uses the Boost library for cross platform abstractions that are not (yet?) available in the C++ Standard Library
    * Precompiled [binaries](https://github.com/r52/op64/wiki/Quick-Guide-to-Compiling-Boost-for-op64) for Visual Studio 2015 for the components used in op64 are available within the repository
    * Set the environment variable `BOOST_ROOT` to the root of the Boost library installation
* [Qt 5.3 or higher](http://www.qt.io/) for the Qt GUI project
    * Install an opensource 64-bit build for your OS and compiler
    * Alternatively, acquire the source code and build Qt with the modules that you want. This option also allows you to [build minimal versions of the Qt libraries](https://github.com/r52/op64/wiki/Quick-Guide-to-Compiling-Qt-for-op64) which removes certain large dependencies such as ICU and WebKit.
    * Set the environment variable `QTDIR` to the root of your Qt installation (contains the include and lib directories)
    * Qt Visual Studio Add-in is recommended for Visual Studio users
* A C++ compiler with C++11/14 support, such as
    * Visual Studio 2015
    * Intel C++ 15
    * GCC 4.7 and later
    * Clang 3.1 and later

## Building on Windows

Visual Studio 2015 and later is a hard requirement as op64 uses C++11/14 features that are not available in compilers older than Visual Studio 2015. Visual Studio 2013 support has been dropped. Intel C++ 15.0 works, but still requires VS2015 due to op64 using features from the C++14 STL. Visual Studio Community editions are supported.

### Compilers Tested

* Visual Studio 2015
* Intel C++ 15.0

### Building

Use the `op64.sln` solution file and compile the op64-qt project

## Building on Linux

Still experimental

### Compilers Tested

* GCC 4.9.2

### Building

Use `cmake` to configure, the `make && make install` to build.

## Plugins

Since the application is 64-bit, plugins must also be compiled in 64-bit to work. This means all legacy 32-bit plugins do not work with op64. This is a hard restriction and there are no workarounds for this.

Plugins based on the zilmar plugin specification are supported. Support for the mupen64plus plugin specification is planned.

### Folder Structure

op64 uses the same folder structure as Project 64, with a top level `Plugins/` directory and 4 subdirectories `Audio/` `GFX/` `Input/` and `RSP/`. Plugins of each type should be placed in the appropriate directory in order for them to be detected.
