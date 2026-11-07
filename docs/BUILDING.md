# Building a native addon

Use this repository's **main** branch. Use **master** for the separate essential Addons repository.

## Requirements

- CMake 3.17 or newer.
- A C++17 compiler and a 32-bit toolchain matching Legacy Server.
- Windows: Visual Studio or Build Tools with the Desktop development with C++ workload.
- Linux: a working i686/multilib C++ toolchain.

## Windows

Run from the template directory:

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release --parallel
```

With the Visual Studio generator, the output is `build/Release/ExampleAddon.dll`. A single-configuration Windows generator places the DLL directly in `build/`.

## Linux

For an x86-64 host configured with multilib:

```sh
CXXFLAGS=-m32 cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

With a single-configuration generator, the output is `build/ExampleAddon.so`. The CMake helper rejects a 64-bit target because it cannot load into the 32-bit server. Linux binaries must be built and tested with a native Linux toolchain; a Windows DLL cannot be renamed to a shared library.

## Add or rename source files

The project defines its target through:

```cmake
include(cmake/LegacyAddon.cmake)
legacy_addon(ExampleAddon src/Main.cpp)
```

Rename `ExampleAddon` there to change the library name. List additional source paths in the same call. Keep the class named in `REGISTER_ADDON(YourClass)` consistent with your implementation; the class and library names do not have to be identical.

## Install

Stop the target server before replacing its loaded addon. Copy the DLL or shared library into that installation's `addons/` directory, then start the server normally and check its log for loader errors.

```text
server-installation/
    server.exe                  # server on Linux
    setup.json
    addons/
        ExampleAddon.dll        # ExampleAddon.so on Linux
    main/
    scripts/
    assets/
    resources/
```

Deployment paths are chosen by the server operator. The template does not copy libraries automatically or depend on a developer-specific checkout path.

## SDK mismatch

If the server reports a missing or mismatched SDK fingerprint, replace all four SDK headers with the matching server's copies and rebuild. Version stays 1; the fingerprint enforces the current native layout. Never alter the fingerprint or add an old-export alias to force a mismatched library to load.
