#include "winutil.h"

namespace winutil {

bool IsValid(HWND hwnd) {
    return hwnd != nullptr && IsWindow(hwnd);
}

HWND ForegroundWindow() {
    return GetForegroundWindow();
}

void EnsureRestored(HWND hwnd) {
    if (!IsValid(hwnd)) {
        return;
    }
    if (IsZoomed(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if ((style & WS_MAXIMIZE) != 0) {
        SetWindowLongPtrW(hwnd, GWL_STYLE, style & ~static_cast<LONG_PTR>(WS_MAXIMIZE));
    }
}

bool GetBounds(HWND hwnd, RECT& out) {
    if (!IsValid(hwnd)) {
        return false;
    }
    return GetWindowRect(hwnd, &out) != FALSE;
}

bool SetBounds(HWND hwnd, const RECT& rc) {
    if (!IsValid(hwnd)) {
        return false;
    }
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;
    return SetWindowPos(hwnd, nullptr, rc.left, rc.top, width, height,
                        SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

bool GetMonitorWorkArea(HWND hwnd, RECT& out) {
    if (!IsValid(hwnd)) {
        return false;
    }
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (GetMonitorInfoW(monitor, &info) == FALSE) {
        return false;
    }
    out = info.rcWork;
    return true;
}

RECT HalfBounds(const RECT& work, Half half) {
    RECT rc = work;
    const int midX = work.left + (work.right - work.left) / 2;
    const int midY = work.top + (work.bottom - work.top) / 2;
    switch (half) {
        case Half::Left:
            rc.right = midX;
            break;
        case Half::Right:
            rc.left = midX;
            break;
        case Half::Top:
            rc.bottom = midY;
            break;
        case Half::Bottom:
            rc.top = midY;
            break;
    }
    return rc;
}

}  // namespace winutil
