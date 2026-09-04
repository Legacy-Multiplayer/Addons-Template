# Building Legacy Server Native Addons

This guide explains how to compile native addons from source on Windows and Linux using CMake.

---

## 1. Prerequisites

### Windows:
- **CMake**: Version 3.15 or higher ([cmake.org](https://cmake.org/download/))
- **Visual Studio**: 2019 or 2022 with the **Desktop development with C++** workload (MSVC v142/v143 toolset)

### Linux:
- **CMake**: Version 3.15 or higher (`sudo apt install cmake` or equivalent)
- **Compiler**: GCC 9+ or Clang 10+ (`sudo apt install build-essential`)

---

## 2. Building on Windows

Open **Developer PowerShell for VS 2022** (or Command Prompt) and run:

```powershell
# 1. Configure the CMake build system
cmake -S . -B build

# 2. Compile the shared library (Release mode)
cmake --build build --config Release --parallel
```

### Build Artifact:
The compiled dynamic library will be generated at:
```
build/Release/ExampleAddon.dll
```
*(or `build/ExampleAddon.dll` depending on the CMake generator)*

---

## 3. Building on Linux

Open a terminal in the project root and run:

```bash
# 1. Configure the CMake build system
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2. Compile the shared library
cmake --build build -j$(nproc)
```

### Build Artifact:
The compiled shared object will be generated at:
```
build/ExampleAddon.so
```

---

## 4. Deploying to Legacy Server

1. Navigate to your Legacy Server installation root.
2. Locate (or create) the `addons/` directory:
   ```
   Legacy-Server/
   ├── server.exe (or server on Linux)
   ├── setup.json
   ├── addons/          <-- Place compiled addon here
   │   └── ExampleAddon.dll (or ExampleAddon.so)
   ├── main/
   └── sidescripts/
   ```
3. Copy `ExampleAddon.dll` (Windows) or `ExampleAddon.so` (Linux) into `addons/`.
4. Start the server. The addon loader will automatically discover, load, and initialize `ExampleAddon`. Check `server.log` to confirm initialization messages.
