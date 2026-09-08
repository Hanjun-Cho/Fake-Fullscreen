#include "winutil.h"

#include <algorithm>
#include <vector>

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

bool GetNeighborMonitorWorkArea(HWND hwnd, Direction dir, RECT& out) {
    if (!IsValid(hwnd)) {
        return false;
    }
    HMONITOR current = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO curInfo{};
    curInfo.cbSize = sizeof(curInfo);
    if (GetMonitorInfoW(current, &curInfo) == FALSE) {
        return false;
    }

    struct Entry {
        HMONITOR handle;
        MONITORINFO info;
    };
    std::vector<Entry> monitors;
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL {
            auto* vec = reinterpret_cast<std::vector<Entry>*>(lParam);
            MONITORINFO mi{};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(hMonitor, &mi) != FALSE) {
                vec->push_back(Entry{hMonitor, mi});
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&monitors));

    const RECT& curR = curInfo.rcWork;
    const int curCx = (curR.left + curR.right) / 2;

    struct Candidate {
        RECT work;
        long yOverlap;   // shared vertical span with the current monitor
        long xDist;      // horizontal distance between monitor centers
    };
    std::vector<Candidate> inDir;
    for (const Entry& e : monitors) {
        const RECT& r = e.info.rcWork;
        const int cx = (r.left + r.right) / 2;
        const bool left = dir == Direction::Left;
        if (left ? (cx >= curCx) : (cx <= curCx)) {
            continue;
        }
        const int top = std::max(curR.top, r.top);
        const int bottom = std::min(curR.bottom, r.bottom);
        const long yOverlap =
            static_cast<long>(std::max(0, bottom - top));
        // Candidates were filtered to strictly lie in `dir`, so this is positive.
        const long xDist = left ? static_cast<long>(curCx - cx)
                                : static_cast<long>(cx - curCx);
        inDir.push_back(Candidate{r, yOverlap, xDist});
    }
    if (inDir.empty()) {
        return false;
    }

    // Prefer the nearest monitor sharing the window's vertical row; only when no
    // monitor vertically overlaps does a diagonally placed one win, by closeness.
    const Candidate* best = nullptr;
    for (const Candidate& c : inDir) {
        const bool better =
            best == nullptr ||
            (c.yOverlap > 0 && best->yOverlap <= 0) ||
            ((c.yOverlap > 0) == (best->yOverlap > 0) && c.xDist < best->xDist);
        if (better) {
            best = &c;
        }
    }
    out = best->work;
    return true;
}

}  // namespace winutil
