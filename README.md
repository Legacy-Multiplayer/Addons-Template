# Legacy native addon template

Build native server plugins with C++17. Derive from `CAddon`, register typed functions and events in `OnLoad()`, then write `LEGACY_ADDON(YourClass)`. The framework owns registrations and cleanup.

```cpp
#include "CAddon.h"

class CExampleAddon final : public CAddon
{
    bool OnLoad() override
    {
        Function("doubleNumber", [](int number)
        {
            return number * 2;
        });
        Event("onPlayerConnect", [this](int playerId)
        {
            Log.Info("Player {} connected", playerId);
        });
        return true;
    }
};

LEGACY_ADDON(CExampleAddon)
```

Lua can call `doubleNumber(21)` and receive `42`. Wrong types and out-of-range integers produce a Lua error naming the function.

## Build

Requirements: CMake 3.17+, C++17, a 32-bit toolchain matching Legacy Server.

Windows:

```sh
cmake -S . -B build -A Win32
cmake --build build --config Release --parallel
```

Linux with multilib installed:

```sh
CXXFLAGS=-m32 cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Place the resulting `ExampleAddon.dll` or `ExampleAddon.so` in the server's `addons/` directory. Rename the target once in `CMakeLists.txt`; `legacy_addon(Name src/Main.cpp)` supplies shared-library settings, SDK includes, and exports.

## SDK layout

The addon API version remains **1**. This update intentionally changes its binary layout. Rebuild every addon with the current headers; previous addon binaries are unsupported. The host checks `GetAddonSDKSignature` before initialization and rejects stale DLLs. `LEGACY_ADDON` generates that export and the standard lifecycle exports automatically. Do not change or copy a signature to bypass the check.

The four SDK headers must come from the same server revision: `Addon.h`, `CAddon.h`, `CAddonValues.h`, and `CAddonResources.h`. No server internals, Lua headers, platform injection code, or external C++ libraries are required.

See [API](docs/API.md) for the helpers and [development](docs/DEVELOPMENT.md) for lifetime and threading rules. The essential addons use this same framework.
