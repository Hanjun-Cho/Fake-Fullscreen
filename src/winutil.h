#pragma once
#include <windows.h>

namespace winutil {

bool IsValid(HWND hwnd);
HWND ForegroundWindow();

// Drops a window out of the OS maximized state so an exact SetWindowPos size
// is honored. While zoomed, Windows pins a window to the work area and ignores
// arbitrary resizes.
void EnsureRestored(HWND hwnd);

bool GetBounds(HWND hwnd, RECT& out);
bool SetBounds(HWND hwnd, const RECT& rc);
bool GetMonitorWorkArea(HWND hwnd, RECT& out);

enum class Half { Left, Right, Top, Bottom };

enum class Direction { Left, Right };

// Work area of the monitor adjacent to the one `hwnd` currently occupies, in
// the given horizontal direction. Returns the nearest monitor sharing the
// window's row when one exists, otherwise the closest monitor in that
// direction. False if hwnd is invalid or no monitor lies in that direction.
bool GetNeighborMonitorWorkArea(HWND hwnd, Direction dir, RECT& out);

}  // namespace winutil
