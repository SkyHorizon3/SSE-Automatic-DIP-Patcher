# Automatic DIP Patcher (SKSE plugin)

Small SKSE plugin to run DIP patches automatically from JSON configs at game start.

## Features
- Auto-detects `DIP.exe` (2.0.2+), searches `Data\\DIP` first then `Data` recursively
- Applies multiple DIP patches in one run via SKSE
- Alphabetic json order
- Writes back resulting JSON after patching (updates `alreadyPatched`)
- Optional debug logging

## Requirements
- Skyrim SE/AE/VR with SKSE64
- Windows 10+
- DIP 2.0.2+ (auto-detected)
- If building:
  - Visual Studio 2022 (MSVC 14.3+), CMake 3.21+
  - vcpkg (manifest mode) and Git submodules

## Configuration
- JSON directory: `Data\\SKSE\\Plugins\\AutomaticPatcher\\DIP`
- INI: `Data\\SKSE\\Plugins\\<PluginName>.ini` with keys:
  - `EnableDebugLog` (bool)
  - `EnableWriteJSON` (bool) – when true, writes a preset JSON
  - `EnablePopupWindow` (bool)
- JSON fields per entry:
  - `patchPath` (string): DIP installation folder containing a `patch` subfolder
  - `alreadyPatched` (bool)

## Usage
1) Place one or more JSON files into `Data\\SKSE\\Plugins\\AutomaticPatcher\\DIP`
2) They are processed in alphabetical order by file name
3) The plugin runs `DIP.exe` with the detected `patchPath` against your `Data` folder
4) Result JSON files are written back with updated `alreadyPatched`

Tip: Set `EnableWriteJSON = true` in the INI to generate a preset JSON with discovered DIP installs.

## Build
- Get submodules
  - `git submodule update --init --recursive`
- Visual Studio generator
  - Configure Debug: `cmake -S . -B build/vs-debug --preset vs-debug`
  - Build Debug: `cmake --build build/vs-debug --config Debug -j`
  - Configure Release: `cmake -S . -B build/vs-release --preset vs-release`
  - Build Release: `cmake --build build/vs-release --config Release -j`
- Output
  - Debug DLL: `build/vs-debug/Debug/AutomaticPatcher.dll`
  - Release DLL: `build/vs-release/Release/AutomaticPatcher.dll`

## Compatibility
- Designed for CommonLibSSE-NG; supports SE/AE/VR
- No known runtime conflicts; relies on `DIP.exe` behavior and patch contents

## Credits
- CommonLibSSE-NG
