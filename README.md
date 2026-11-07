# Native addon template

Create a native Legacy Server plugin with C++17. This repository uses **`main`**; the essential Addons repository uses **`master`**.

Derive from `CAddon`, register functions/events in `OnLoad()`, and finish with `REGISTER_ADDON(YourClass)`. The framework generates the exports and owns registrations, timers and cleanup.

## Starter

The following is the actual `src/Main.cpp` shipped by this template:

```cpp
#include "CAddon.h"

class CExampleAddon final : public CAddon
{
    bool OnLoad() override
    {
        Function("hello", [](std::optional<std::string> name)
        {
            return "Hello, " + name.value_or("Developer") + "!";
        });

        Event("onPlayerConnect", [this](int playerId)
        {
            const auto player = Players.Find(playerId);

            if (player.IsValid())
            {
                Log.Info("Player {} connected", player.Id());
            }
        });

        Once("onMainScriptInit", [this]()
        {
            Log.Info("The game script is ready");
        });

        return true;
    }
};

REGISTER_ADDON(CExampleAddon)
```

Lua can call `hello("Kavnn")` and receive `"Hello, Kavnn!"`. Calling `hello()` uses `"Developer"`. Wrong argument types produce an error naming the function. `examples/example.lua` demonstrates these same calls.

`Event` callbacks may select a typed prefix of event arguments. `Once` removes its subscription before delivery, including recursive delivery. Entity lookup in the connection callback uses the human-only `Players` collection.

## Build and install

Requirements: CMake 3.17+, C++17 and a 32-bit toolchain matching the server.

Windows:

```sh
cmake -S . -B build -A Win32
cmake --build build --config Release --parallel
```

Linux with a multilib toolchain:

```sh
CXXFLAGS=-m32 cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Copy `build/Release/ExampleAddon.dll` (Visual Studio) or `build/ExampleAddon.so` (single-configuration Linux build) into the target server's `addons/` folder while it is stopped. The template does not deploy automatically or require a particular server folder on your computer.

Change the target name in `CMakeLists.txt`. The `legacy_addon(Name src/Main.cpp)` helper supplies SDK includes and shared-library settings; add further source paths to that call when needed.

## SDK layout and formatting

Addon API version remains **1**. The current SDK deliberately does not support previous binary layouts. Rebuild old addons; the server validates the SDK fingerprint before initialization. `REGISTER_ADDON` generates the required fingerprint and lifecycle exports. Do not bypass the check.

Keep these four headers from the same server revision together: `Addon.h`, `CAddon.h`, `CAddonValues.h` and `CAddonResources.h`. The server, essentials and this template use identical copies. No internal server or Lua headers are required for a plugin.

The checked-in `.clang-format` preserves four-space indentation, braces on their own lines, separate function definitions, and the declaration/logical-step spacing used by the SDK. Follow the starter rather than copying old manual export or registration boilerplate.

## Reference

- [API](docs/API.md): actual helper signatures, results and examples.
- [Development](docs/DEVELOPMENT.md): ownership, callbacks and threading rules.
- [Building](docs/BUILDING.md): compiler, output and installation details.

The essential addons use this same framework. `API()` remains available for native functions without a convenience helper.
