# FakeFullscreen

Global-hotkey window manager: maximize the focused window to the monitor it
occupies, or snap it to a half. All placements respect a margin around the
monitor's working area.

| Action | Default hotkey | Effect |
| ------ | -------------- | ------ |
| `maximize_toggle` | `Ctrl+Alt+Space` | Fill the focused window's monitor work area (minus margins); press again to restore its original bounds. |
| `snap_left` | `Ctrl+Alt+Left` | Move the window into the left half (minus margins). No-op if already there. |
| `snap_right` | `Ctrl+Alt+Right` | Move into the right half. No-op if already there. |
| `snap_top` | `Ctrl+Alt+Up` | Move into the top half. No-op if already there. |
| `snap_bottom` | `Ctrl+Alt+Down` | Move into the bottom half. No-op if already there. |

`maximize_toggle` toggles back to the original bounds; snaps are one-way and
idempotent (snapping to the same half again does nothing, and does not restore
the previous size).

Hotkeys are remapped by editing `config.ini`, which sits **next to the
executable in the build directory**. No recompile needed to remap keys; the
set of available actions is compiled in.

## Layout

```
├── CMakeLists.txt       # CMake build
├── config.ini           # sample bindings (copied into build/ on first run)
├── run.bat              # configure + build + run
├── src/
│   ├── main.cpp         # entry point: load config, run message loop
│   ├── winutil.*        # winutil: HWND/monitor/rect Win32 helpers
│   ├── Actions.*        # ffs: Action interface, actions, factory
│   ├── Config.*         # parse config.ini (no third-party deps)
│   └── HotkeyManager.*  # register + dispatch hotkeys to actions
```

## Adding a new action

1. Subclass `ffs::Action` (or `ffs::ToggleBoundsAction`) in `src/Actions.*`.
2. Register it in `ffs::ActionFactory::Create` under a name.
3. Reference that name from `build/config.ini`.

## Build & run

Requires [CMake](https://cmake.org) and a MinGW/g++ toolchain on `PATH`.
Then double-click `run.bat`, or:

```bat
run.bat
```

It configures, builds, and launches `build\fakefullscreen.exe`. No console
window appears — the app lives in the **system tray**. Right-click its tray
icon and choose **Exit FakeFullscreen** to quit fully.

## Configuration (build/config.ini)

```
maximize_toggle = ctrl+alt+space
snap_left       = ctrl+alt+left
```

- `action = modifiers+key`
- Modifiers: `ctrl`, `alt`, `shift`, `win` (combined with `+`).
- Keys: letters, digits, `space`, `left/right/up/down`, `f1`–`f24`, and others
  (see `KeyTokenToVk` in `src/Config.cpp`).

> The margin for placements is compiled in (`kDefaultMargin` in
> `src/Actions.cpp`). Remapping keys does not require a rebuild; changing the
> margin or adding an action kind does.
