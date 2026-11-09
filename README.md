# Native addon template

Create a native Legacy Server plugin with C++17. This repository uses **`main`**; the essential Addons repository uses **`master`**.

Derive from `SDK::CPlugin`, register functions/events in `OnLoad()`, and finish with `REGISTER_PLUGIN(YourClass)`. The framework generates the exports and owns registrations, timers and cleanup.

## Starter

The following is the actual `src/Main.cpp` shipped by this template:

```cpp
#include "SDK/Plugin.h"

using namespace SDK;

class CExampleAddon final : public CPlugin
{
    bool OnLoad() override
    {
        Function("hello", [](std::optional<std::string> name)
        {
            return "Hello, " + name.value_or("Developer") + "!";
        });

        Function("echoOptions", [](CTable options)
        {
            return options;
        });

        Function("countOptionsLater", [this](CTable options, CCallback callback)
        {
            Async([options](CCancellation cancellation)
            {
                return cancellation.IsCancelled() ? size_t{0} : options.Size();
            }, [callback](size_t count)
            {
                CTable result;
                result.Set("count", count);
                callback.Invoke(result);
            });

            return true;
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

REGISTER_PLUGIN(CExampleAddon)

```

Lua can call `hello("Kavnn")` and receive `"Hello, Kavnn!"`. Calling `hello()` uses `"Developer"`. Wrong argument types produce an error naming the function. `examples/example.lua` demonstrates these same calls. `echoOptions` round-trips an ordinary Lua table; `countOptionsLater(options, callback)` demonstrates owned table snapshots and a callback delivered after background work. See `examples/tables.lua`.

`Event` callbacks may select a typed prefix of event arguments. `Once` removes its subscription before delivery, including recursive delivery. Entity lookup in the connection callback uses the human-only `Players` collection.

## Build and install

Clone with `git clone --recurse-submodules https://github.com/Legacy-Multiplayer/Addons-Template.git`. For an existing checkout, run `git submodule update --init --recursive`.

Requirements: CMake 3.20+, C++17 and a 32-bit toolchain matching the server.

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

Change the target name in `CMakeLists.txt`. The `legacy_plugin(Name src/Main.cpp)` helper supplies SDK includes and shared-library settings; add further source paths to that call when needed.

## SDK layout and formatting

Addon API version remains **1**. The current SDK deliberately does not support previous binary layouts. Rebuild old addons; the server validates the SDK fingerprint before initialization. `REGISTER_PLUGIN` generates the required fingerprint and lifecycle exports. Do not bypass the check.

The `SDK` Git submodule pins [Addon-SDK](https://github.com/Legacy-Multiplayer/Addon-SDK). Its `include/SDK/Native/` directory separates player, vehicle, actor, object, audio and other native tables. Managed C++ helpers live in `SDK/Plugin.h`, `SDK/Table.h`, `SDK/Callback.h`, `SDK/Entity.h` and the other focused headers. All C++ helpers use `namespace SDK`. No internal server or Lua headers are required.

The checked-in `.clang-format` preserves four-space indentation, braces on their own lines, separate function definitions, and the declaration/logical-step spacing used by the SDK. Follow the starter rather than copying old manual export or registration boilerplate.

## Reference

- [API](docs/API.md): actual helper signatures, results and examples.
- [Development](docs/DEVELOPMENT.md): ownership, callbacks and threading rules.
- [Building](docs/BUILDING.md): compiler, output and installation details.

The essential addons use this same framework. `API()` remains available for native functions without a convenience helper.
