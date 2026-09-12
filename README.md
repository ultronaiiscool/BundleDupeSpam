# BundleDupeSpam

Minecraft Bedrock Edition (Windows) internal DLL that spams `ItemStackRequest` Take actions as fast as the game allows.

Designed for the dropper + bundle duplication setup where normal inventory clicks fail because the container state flips too quickly. Sending the Take packets in a tight loop bypasses the UI timing.

## Features

- ImGui overlay (toggle with **Insert**)
- Configurable delay (0 = maximum speed)
- Configurable dropper slot + take count
- Packet builder for `ItemStackRequest` with Take action
- Clean unload with **End** key

## Build

### Requirements
- Visual Studio 2022 (or Build Tools) with C++ desktop workload
- CMake 3.15+
- Windows 10/11 SDK

### Steps

```bash
git clone https://github.com/ultronaiiscool/BundleDupeSpam.git
cd BundleDupeSpam

# Grab dependencies
# - imgui (docking or master) into imgui/
# - MinHook into minhook/

mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

The resulting `BundleDupeSpam.dll` will be in `build/Release/`.

## Injection

1. Start Minecraft Bedrock (Windows 10/11 edition / GDK).
2. Inject `BundleDupeSpam.dll` with any injector that supports UWP/GDK processes (Extreme Injector, Process Hacker, custom, etc.).
3. Once in-game, open the dropper that contains the bundle.
4. Press **Insert** to show the GUI.
5. Set the correct slot of the bundle inside the dropper.
6. Enable spam. Adjust delay if the server starts rejecting requests.
7. Press **End** to unload cleanly.

## How it works

The classic bundle-in-dropper dupe relies on the container contents changing while the player is trying to take an item out. The game’s normal click handling is too slow / state-gated, so the take often fails.

This DLL builds and queues `ItemStackRequest` packets containing a single **Take** action:

- Source = the open container (dropper) + the configured slot
- Destination = cursor
- Count = configurable

The spam loop runs on its own thread and can fire packets every tick or faster (delay 0). Because the packets go through the network layer instead of the UI click path, they are not blocked by the same timing checks.

**Note:** Exact container IDs, stack network IDs, and the real network send function are version-dependent. The included builder uses common values. For production use you should:

1. Pattern-scan the real `NetworkHandler` send function for your current game version.
2. Capture a legitimate Take packet while the dropper is open (using a packet logger) and reuse the container / stack IDs it contains.
3. Update the offsets / signatures in `packet.cpp` and `hooks.cpp`.

## Safety / Notes

- Singleplayer / realm / server use is at your own risk.
- Many public servers ban for inventory exploits.
- The DLL does not modify world files; it only sends client packets.
- Unload with End before closing the game to avoid crashes.

## Credits

- ImGui
- MinHook
- Protocol research from gophertunnel, PocketMine-MP, Mojang bedrock-protocol-docs

Made by discord.gg/mXXQSYSVYq
