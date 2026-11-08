A **C++17 template for native Legacy Multiplayer server plugins**, with a shared SDK, working examples and Windows/Linux builds.

## Included

- `SDK::CPlugin` and `REGISTER_PLUGIN`: managed initialization, entry-point exports and cleanup.
- Typed Lua functions with checked arguments, optional values and multiple return values.
- `SDK::CTable` and `SDK::CCallback`: owned table data and retained Lua callbacks, without a separate Lua adapter.
- Events, one-shot/repeating timers and background work with main-thread completion.
- Logging, resource-relative files, configuration and generation-checked entity references.
- A pinned [Addon-SDK](https://github.com/Legacy-Multiplayer/Addon-SDK) submodule, with subsystem headers for players, actors, vehicles, objects and other services.
- Starter examples for greetings, table round-tripping and asynchronous callbacks, plus API and development guides.

## Downloads

| Package | Platform | Example library |
| --- | --- | --- |
| `ExampleAddon-windows-x86.zip` | Windows, 32-bit | `ExampleAddon.dll` |
| `ExampleAddon-linux-x86.tar.gz` | Linux, 32-bit | `ExampleAddon.so` |

These are example-plugin binaries, accompanied by documentation, Lua examples and a build manifest. To develop your own plugin, clone the source **including its SDK submodule**:

```sh
git clone --recurse-submodules https://github.com/Legacy-Multiplayer/Addons-Template.git
```

GitHub's automatic source archives omit submodule contents. For an existing checkout, run `git submodule update --init --recursive`.

## Your first function

Inside your plugin's `OnLoad()`:

```cpp
Function("greet", [](std::string name)
{
    return "Hello, " + name + "!";
});
```

Call it from server Lua:

```lua
print(greet("Developer"))
```

Rename the target in `CMakeLists.txt`, implement your features, and build with **CMake 3.20+, C++17 and a 32-bit toolchain**. Place the resulting library in a stopped server's `addons` directory and enable it in `setup.json`.

## Compatibility

- Native addon API version remains **1**. The exact SDK fingerprint must match the server; an older binary is not compatible merely because it also reports version 1.
- Windows packages require the matching **x86 MSVC runtime**. Linux builds use an **Ubuntu 22.04 x86** baseline and require compatible 32-bit C/C++ runtimes.
- Lua, networking and live entity operations belong on the server thread. Workers operate on independent data and must cooperate with cancellation.
- Both platforms are built by GitHub Actions. Each package identifies its source and SDK commits in `build-manifest.json`.

**Documentation:** [API](https://github.com/Legacy-Multiplayer/Addons-Template/blob/main/docs/API.md) · [Development](https://github.com/Legacy-Multiplayer/Addons-Template/blob/main/docs/DEVELOPMENT.md) · [Building](https://github.com/Legacy-Multiplayer/Addons-Template/blob/main/docs/BUILDING.md)
