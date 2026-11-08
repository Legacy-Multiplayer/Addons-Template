# Building a native addon

Use this repository's **main** branch. Use **master** for the separate essential Addons repository.

## Requirements

- CMake 3.20 or newer.
- A C++17 compiler and a 32-bit toolchain matching Legacy Server.
- Windows: Visual Studio or Build Tools with the Desktop development with C++ workload.
- Linux: a working i686/multilib C++ toolchain.

Initialize the pinned SDK first with `git submodule update --init --recursive`, or clone using `--recurse-submodules`.

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
add_subdirectory(SDK)
include(SDK/cmake/Plugin.cmake)
legacy_plugin(ExampleAddon src/Main.cpp)
```

Rename `ExampleAddon` there to change the library name. List additional source paths in the same call. Keep the class named in `REGISTER_PLUGIN(YourClass)` consistent with your implementation; the class and library names do not have to be identical.

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

If the server reports a missing or mismatched SDK fingerprint, update the pinned `SDK` submodule to the commit used by the server and rebuild. Version stays 1; the fingerprint enforces the current native layout. Never alter the fingerprint or add an old-export alias to force a mismatched library to load.

## GitHub builds and draft releases

Pushes and pull requests on `main` build Windows x86 and Linux x86. The workflow checks binary architecture and includes a build manifest with source/SDK commits in each package. Linux uses Ubuntu 22.04 with multilib; Windows uses Visual Studio's Win32 toolchain.

Run **Build plugins** manually on `main` with a new version such as `v1.0.0` to prepare a draft after both builds pass. Leave the tag empty for build artifacts only. The release job uses the repository's `GITHUB_TOKEN` with contents-write permission; no personal token is stored. Actions must be enabled by the organization. Review the draft before publishing; the workflow never publishes automatically.

Push the SDK commit first, then consumer repositories. Recursive checkout must be able to fetch the pinned SDK revision. Updating a submodule requires a reviewed commit in each consumer; builds do not track SDK `main` automatically.
