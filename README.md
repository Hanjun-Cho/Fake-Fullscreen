# FakeFullscreen

Global-hotkey window manager: maximize the focused window to the monitor it
occupies, or snap it to a half. All placements respect a margin around the
monitor's working area.

| Action | Default hotkey | Effect |
| ------ | -------------- | ------ |
| `maximize_toggle` | `Ctrl+Alt+Space` | Fill the focused window's monitor work area (minus margins); press again to restore its original size and position. |
| `snap_left` | `Ctrl+Alt+Left` | Pin the window to the left, splitting that axis in half. |
| `snap_right` | `Ctrl+Alt+Right` | Pin the window to the right, splitting that axis in half. |
| `snap_top` | `Ctrl+Alt+Up` | Pin the window to the top, splitting that axis in half. |
| `snap_bottom` | `Ctrl+Alt+Down` | Pin the window to the bottom, splitting that axis in half. |
| `move_monitor_left` | `Ctrl+Alt+Shift+Left` | Move the window to the monitor on the left, keeping its snapped region. |
| `move_monitor_right` | `Ctrl+Alt+Shift+Right` | Move the window to the monitor on the right, keeping its snapped region. |

## Snapping

Snaps are cumulative: each snap pins the window to one side of a single axis
(left/right or top/bottom). Snapping a second, perpendicular side composes the
two into a **quarter** of the monitor, so `snap_right` followed by `snap_bottom`
places the window in the bottom-right quadrant (one quarter the size of
`maximize_toggle`), and vice versa.

Re-pressing the side a window already hugs **expands** it rather than doing
nothing: a quarter returns to that half, and a half returns to fill the whole
monitor. Reaching the full monitor returns it to the "not snapped" state for
that axis.

`maximize_toggle` is the only action that restores by re-pressing. A window the
app controls (maximized or snapped) is released from app control only when you
actually **drag it by its title bar**; clicking it anywhere — including its
client area — never affects it. Dragging does not resize the window; it simply
lets Windows move it as a normal window (no jump). Snaps are otherwise one-way:
they never restore a previous size on their own.

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
maximize_toggle     = ctrl+alt+space
snap_left           = ctrl+alt+left
move_monitor_left   = ctrl+alt+shift+left
```

- `action = modifiers+key`
- Modifiers: `ctrl`, `alt`, `shift`, `win` (combined with `+`).
- Keys: letters, digits, `space`, `left/right/up/down`, `f1`–`f24`, and others
  (see `KeyTokenToVk` in `src/Config.cpp`).

> The margin for placements is compiled in (`kDefaultMargin` in
> `src/Actions.cpp`). Remapping keys does not require a rebuild; changing the
> margin or adding an action kind does.
