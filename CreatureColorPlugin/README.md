# Creature Color Plugin - Complete Example

## What This Plugin Does

This plugin solves the **exact problem** you described:
- ✅ Targets the creature you're looking at (crosshair aim)
- ✅ Checks if you own/have permission to modify it
- ✅ Colors the specified body region
- ✅ Updates **immediately** (no cryo/render distance required)
- ✅ Only works on creatures you own (tribe or personal ownership)

## Usage

In-game, aim at a tamed creature you own and type:
```
/colorregion <region> <colorid>
```

**Example:**
```
/colorregion 0 5     # Color region 0 with color ID 5
/colorregion 4 137   # Color region 4 with color ID 137
```

**Regions:** 0-5 (6 total color regions)
**Color IDs:** 0-226 (ARK's color palette)

## How It Solves Your Problems

### 1. Targeting - The Right Way

**Your problem:** Couldn't target specific creatures reliably

**Solution:** Using `UShooterCheatManager::GetAimedTargetFromLocation()`

```cpp
// Get player's view point
UE::Math::TVector<double> view_location;
UE::Math::TRotator<double> view_rotation;
player_controller->GetPlayerViewPoint(&view_location, &view_rotation);

// Get the actor under crosshair
UShooterCheatManager* cheat_mgr = AsaApi::GetApiUtils().GetCheatManagerByPC(player_controller);
AActor* aimed_actor = cheat_mgr->GetAimedTargetFromLocation(&view_location, &view_rotation, nullptr);
```

This is **exactly** how ARK admin commands target creatures. Line 9654 in Actor.h.

### 2. Ownership Checking - Two Methods

**Your problem:** Changes applied to any creature, not just owned ones

**Solution:** Check both tribe ID and direct ownership

```cpp
// Method 1: Tribe ownership
int player_tribe_id = AsaApi::GetApiUtils().GetTribeID(player_controller);
int dino_tribe_id = dino->TamingTeamIDField();

if (player_tribe_id != 0 && player_tribe_id == dino_tribe_id) {
    // Same tribe - allow
}

// Method 2: Direct ownership (for tribeless players)
unsigned __int64 player_id = AsaApi::GetApiUtils().GetPlayerID(player_controller);
unsigned int dino_owner_id = dino->OwningPlayerIDField();

if (player_id == dino_owner_id) {
    // Direct owner - allow
}
```

See APrimalDinoCharacter fields at line 7601-7603 in Actor.h.

### 3. Color Change - Direct Memory Access

**Your problem:** Using admin commands required console, didn't update properly

**Solution:** Direct manipulation of ColorSetIndicesField

```cpp
// Set color directly (no admin command needed!)
dino->ColorSetIndicesField()[region] = static_cast<unsigned __int8>(color_id);
```

This is the **actual memory field** that stores creature colors. Line 7532 in Actor.h.

### 4. Instant Update - The Critical Fix

**Your problem:** Creature wouldn't update until cryo/render distance

**Solution:** Force network replication

```cpp
// Force immediate network update
dino->ForceNetUpdate(false, true, false);  // bAbsoluteForceNetUpdate = true

// Also mark render state dirty
dino->MarkComponentsRenderStateDirty();
```

This forces the server to send the update to all clients **immediately**.

## How to Identify Hooks WITHOUT the Creator Tool

### The Hook System Explained

Hooks in this API work by **intercepting game function calls**. Here's the process:

#### Step 1: Declare the Hook

```cpp
DECLARE_HOOK(FunctionName, ReturnType, Param1Type, Param2Type, ...);
```

This creates:
- A function pointer type
- An `_original` function pointer to call the original

#### Step 2: Implement the Hook

```cpp
ReturnType Hook_FunctionName(Param1Type param1, Param2Type param2, ...) {
    // Your code BEFORE original function

    // Call original
    ReturnType result = FunctionName_original(param1, param2, ...);

    // Your code AFTER original function
    return result;
}
```

#### Step 3: Register the Hook

```cpp
AsaApi::GetHooks().SetHook(
    "ClassName.MethodName(ParamType1,ParamType2&,...)",
    &Hook_FunctionName,
    &FunctionName_original
);
```

### Finding Hook Signatures Without the Tool

**The hard truth:** Without the hook creator tool, you need to:

1. **Use existing hooks** - See `AsaApi/Core/Private/Ark/HooksImpl.cpp` for all available hooks
2. **Reverse engineer the game executable** - Use tools like IDA Pro, Ghidra, or x64dbg
3. **Read the game's symbols** - If Studio Wildcard provides PDB files
4. **Study the API headers** - The `Actor.h` file shows function signatures

**But here's the secret:** You don't always need new hooks!

### Work Smarter: Use Existing Entry Points

Instead of creating hooks, use the existing command system:

```cpp
// Chat command - triggers when player types /command
AsaApi::GetCommands().AddChatCommand("/mycommand", callback);

// Console command
AsaApi::GetCommands().AddConsoleCommand("mycommand", callback);

// RCON command
AsaApi::GetCommands().AddRconCommand("mycommand", callback);

// Tick callback - runs every frame
AsaApi::GetCommands().AddOnTickCallback("myid", callback);

// Timer callback - runs every second
AsaApi::GetCommands().AddOnTimerCallback("myid", callback);
```

These give you access to the game's internals **without needing custom hooks**.

### Currently Available Hooks (HooksImpl.cpp)

```cpp
// Engine initialization
UEngine_Init

// World tick (every frame)
UWorld_Tick

// Game mode events
AShooterGameMode_InitGame
AShooterGameMode_BeginPlay
AShooterGameMode_Logout

// Player events
AShooterPlayerController_ServerSendChatMessage_Impl
AShooterPlayerController_ConsoleCommand
AShooterPlayerController_OnPossess
AShooterGameMode_HandleNewPlayer_Implementation

// RCON events
RCONClientConnection_ProcessRCONPacket
URCONServer_Init

// Timer
AGameState_DefaultTimer

// Broadcast
UShooterCheatManager_Broadcast
```

## Alternative Targeting Methods

If crosshair targeting isn't what you need:

### Get All Creatures in Radius

```cpp
TArray<AActor*> actors;
FVector player_pos = AsaApi::GetApiUtils().GetPosition(player_controller);

// Get all dinos within 10000 units
AsaApi::GetApiUtils().GetAllActorsInRange(
    &actors,
    player_pos,
    10000.0f,
    EServerOctreeGroup::DINOPAWNS
);

for (AActor* actor : actors) {
    if (actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
        auto* dino = static_cast<APrimalDinoCharacter*>(actor);
        // Process each dino
    }
}
```

### Iterate ALL Actors in World

```cpp
UWorld* world = AsaApi::GetApiUtils().GetWorld();
TArray<AActor*>& all_actors = world->PersistentLevelField()->ActorsField();

for (AActor* actor : all_actors) {
    if (actor && actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
        auto* dino = static_cast<APrimalDinoCharacter*>(actor);
        // Process each dino
    }
}
```

## Key API Locations Reference

- **Actor definitions:** `AsaApi/Core/Public/API/ARK/Actor.h`
- **Utility functions:** `AsaApi/Core/Public/Ark/ArkApiUtils.h`
- **Enums:** `AsaApi/Core/Public/API/Enums.h`
- **Hooks implementation:** `AsaApi/Core/Private/Ark/HooksImpl.cpp`
- **Commands interface:** `AsaApi/Core/Public/ICommands.h`
- **Hooks interface:** `AsaApi/Core/Public/IHooks.h`

## Building the Plugin

### Windows (Visual Studio)

1. Open AsaApi.sln
2. Add this project to the solution
3. Set include directories to point to `AsaApi/Core/Public`
4. Link against `AsaApi.lib`
5. Build as DLL

### Or use CMake

```bash
cd CreatureColorPlugin
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Installation

1. Build the plugin to get `CreatureColorPlugin.dll`
2. Copy to: `<ARK Server>/ShooterGame/Binaries/Win64/ArkApi/Plugins/CreatureColorPlugin/`
3. Copy `PluginInfo.json` to the same directory
4. Restart server

## Why Your Previous Attempts Failed

### Problem 1: Using Admin Commands via Console

Admin commands like `SetTargetDinoColor` require:
- Active targeting (looking at the creature)
- Execution through console/cheat manager
- Proper network context

**Solution:** Direct field manipulation bypasses all this.

### Problem 2: No Network Update

Setting the color field doesn't automatically replicate to clients.

**Solution:** `ForceNetUpdate(false, true, false)` with `bAbsoluteForceNetUpdate=true`

### Problem 3: Permission Checking

The admin commands don't check ownership - they work on anything.

**Solution:** Manual tribe ID and owner ID comparison before modification.

### Problem 4: Wrong Targeting Method

Trying to use generic actor iteration or proximity searches.

**Solution:** `GetAimedTargetFromLocation()` is the **exact** method ARK uses for crosshair targeting.

## Next Steps: RCON Acknowledgement Plugin

Now that you see how this API works, the RCON acknowledgement plugin becomes straightforward:

1. Hook `RCONClientConnection_ProcessRCONPacket` (already exists!)
2. Send acknowledgement via `rcon_client->SendMessageW()`
3. Track command execution
4. Send success/failure response

Want me to build that next?
