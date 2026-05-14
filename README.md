# CS2 Internal

Internal cheat for Counter-Strike 2 featuring a CreateMove-based aimbot and ESP overlay.

## Features

### Aimbot
- **Bone-based targeting** — reads bone matrix directly from `CSkeletonInstance` for accurate head tracking
- **CreateMove hook** — hooks `sub_85DDB0` via MinHook for reliable angle injection
- **Smooth aim** — configurable smoothing with deadzone to prevent jitter
- **FOV limit** — only targets within configurable field of view
- **Team check** — toggleable team filtering
- **Silent aim** — optional silent aim mode

### ESP
- **Box** — standard, corner, and filled box styles
- **Health bar** — color-coded health display
- **Name & Distance** — player identification and range info
- **Customizable colors** — all elements have adjustable colors

### Misc
- **Watermark** — FPS, ping, and cheat name overlay
- **Config system** — save/load settings to `.cfg` file

## Build

### Requirements
- Visual Studio 2022 Build Tools (MSVC v143)
- Windows SDK 10.0

### Cheat DLL
```
build_dll.bat
```

### Injector
```
build_injector.bat
```

## Usage

1. Build the DLL and injector using the provided batch files
2. Launch CS2
3. Run `injector.exe` and select `cs2_internal.dll`
4. Press **INSERT** to open the menu

### Keybinds
| Key | Action |
|-----|--------|
| INSERT | Toggle menu |
| END | Unload cheat |

## Project Structure

```
ImGui DirectX 11 Kiero Hook/
├── main.cpp              # Entry point, D3D11 Present hook, ImGui setup
├── createmove.h          # CreateMove hook + aimbot logic
├── entity.h              # Entity reading, bone positions, team resolution
├── esp.h                 # ESP rendering (boxes, health, names)
├── aimbot.h              # Aimbot settings & key bindings
├── vector.h              # Vector3, ViewMatrix, WorldToScreen
├── game_offsets.h        # Schema offsets & runtime globals
├── cs2_runtime.h         # SchemaSystem init & offset resolution
├── schema_system.h       # SchemaSystem vtable interface
├── pattern_scan.h        # Signature scanning engine
├── includes.h            # Common headers & settings namespace
├── imgui/                # Dear ImGui (v1.90)
└── kiero/                # Kiero D3D hook + MinHook
```

## Technical Details

- **Hook method**: MinHook on `client.dll + 0x85ddb0` (CreateMove)
- **Rendering**: Kiero D3D11 Present hook + ImGui
- **Bone matrix**: `pawn + 0x1D80` (stride 0x20, XYZ at [0..2])
- **Head bone**: index 0
- **Eye position**: `sceneNode.m_vecAbsOrigin + pawn.m_vecViewOffset.z` (offset 0xE70 + 0x20)

## Disclaimer

This project is for educational purposes only. Use at your own risk.
