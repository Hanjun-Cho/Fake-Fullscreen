#include "Actions.h"

#include <cstdio>
#include <unordered_map>

namespace ffs {
namespace {

// Per-edge margin (pixels) inset from each side of the monitor's working area
// before maximizing or snapping, so windows do not touch the screen edges.
constexpr long kDefaultMargin = 15;

// How far the window spans each axis. "Full" means it spans that whole axis;
// Left/Right/Top/Bottom pin it to one half (or quarter when both axes are set).
enum class HSide { Full, Left, Right };
enum class VSide { Full, Top, Bottom };

enum class Mode { Maximize, Snap };

struct Control {
    Mode mode;
    RECT original;  // size (and position) the window had before placement
    RECT applied;   // bounds the app last set for the window
    HSide h;        // current horizontal placement (Snap only)
    VSide v;        // current vertical placement (Snap only)
};

// Windows currently under application control. Shared by all actions so a
// maximized window that is then snapped keeps one "controlled" record.
std::unordered_map<HWND, Control> g_controlled;

bool LogWinError(const char* what) {
    std::fprintf(stderr, "%s failed (error %lu)\n", what, GetLastError());
    return false;
}

bool GetWorkArea(HWND hwnd, RECT& out) {
    return winutil::GetMonitorWorkArea(hwnd, out);
}

bool ApplyMargins(RECT& rc) {
    rc.left += kDefaultMargin;
    rc.top += kDefaultMargin;
    rc.right -= kDefaultMargin;
    rc.bottom -= kDefaultMargin;
    return rc.right > rc.left && rc.bottom > rc.top;
}

bool BoundsEqual(const RECT& a, const RECT& b) {
    return a.left == b.left && a.top == b.top &&
           a.right == b.right && a.bottom == b.bottom;
}

// A rect anchored at `current`'s top-left but sized to `model`. Used to scale
// a window down to its original size without moving it.
RECT SizeAt(const RECT& current, const RECT& model) {
    RECT rc = current;
    rc.right = rc.left + (model.right - model.left);
    rc.bottom = rc.top + (model.bottom - model.top);
    return rc;
}

RECT MaximizeTarget(HWND hwnd, bool& ok) {
    RECT work{};
    if (!GetWorkArea(hwnd, work)) {
        LogWinError("GetWorkArea");
        ok = false;
        return {};
    }
    if (!ApplyMargins(work)) {
        std::fprintf(stderr, "maximize_toggle: margins leave no usable space.\n");
        ok = false;
        return {};
    }
    ok = true;
    return work;
}

RECT MarginedWorkArea(HWND hwnd, bool& ok, const char* what) {
    RECT work{};
    if (!GetWorkArea(hwnd, work)) {
        LogWinError("GetWorkArea");
        ok = false;
        return {};
    }
    if (!ApplyMargins(work)) {
        std::fprintf(stderr, "%s: margins leave no usable space.\n", what);
        ok = false;
        return {};
    }
    ok = true;
    return work;
}

// The rect covered by a region (half or quarter) cut from the margined work
// area. Splitting at the middle of each axis, "Full" spans the whole axis and a
// half-side spans up to the split; adjacent regions only touch along an edge.
RECT RegionBounds(const RECT& work, HSide h, VSide v) {
    RECT rc = work;
    const int midX = work.left + (work.right - work.left) / 2;
    const int midY = work.top + (work.bottom - work.top) / 2;
    switch (h) {
        case HSide::Left:
            rc.right = midX;
            break;
        case HSide::Right:
            rc.left = midX;
            break;
        case HSide::Full:
            break;
    }
    switch (v) {
        case VSide::Top:
            rc.bottom = midY;
            break;
        case VSide::Bottom:
            rc.top = midY;
            break;
        case VSide::Full:
            break;
    }
    return rc;
}

bool IsHorizontalHalf(winutil::Half half) {
    return half == winutil::Half::Left || half == winutil::Half::Right;
}

HSide HorizontalSide(winutil::Half half) {
    return half == winutil::Half::Left ? HSide::Left : HSide::Right;
}

VSide VerticalSide(winutil::Half half) {
    return half == winutil::Half::Top ? VSide::Top : VSide::Bottom;
}

// Pressing `half` evolves the current region (ph, pv) into the next one. The
// pressed arrow sets its own axis to that side, keeping the other axis, so two
// perpendicular snaps compose into a quarter. Re-pressing the side the window
// already hugs expands the orthogonal axis back toward full (quarter -> half ->
// full) rather than doing nothing.
void EvolveRegion(winutil::Half half, HSide ph, VSide pv, HSide& nh, VSide& nv) {
    if (IsHorizontalHalf(half)) {
        const HSide s = HorizontalSide(half);
        if (ph == s && pv != VSide::Full) {
            nh = s;
            nv = VSide::Full;  // quarter -> half
        } else if (ph == s) {
            nh = HSide::Full;  // half -> full
            nv = VSide::Full;
        } else {
            nh = s;            // set horizontal side, keep vertical
            nv = pv;
        }
        return;
    }
    const VSide t = VerticalSide(half);
    if (pv == t && ph != HSide::Full) {
        nh = HSide::Full;  // quarter -> half
        nv = t;
    } else if (pv == t) {
        nh = HSide::Full;  // half -> full
        nv = VSide::Full;
    } else {
        nh = ph;           // set vertical side, keep horizontal
        nv = t;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// MaximizeToggle
// ---------------------------------------------------------------------------

const std::string& MaximizeToggle::ActionName() {
    static const std::string kName = "maximize_toggle";
    return kName;
}

void MaximizeToggle::Apply(HWND hwnd) {
    RECT current{};
    if (!winutil::GetBounds(hwnd, current)) {
        LogWinError("GetBounds");
        return;
    }

    auto it = g_controlled.find(hwnd);

    if (it != g_controlled.end() && it->second.mode == Mode::Maximize) {
        if (BoundsEqual(current, it->second.applied)) {
            // Still maximized (not dragged): toggle off, restore size+position.
            if (winutil::SetBounds(hwnd, it->second.original)) {
                std::printf("%s: restored window to %ldx%ld at (%ld,%ld)\n",
                            Name().c_str(),
                            it->second.original.right - it->second.original.left,
                            it->second.original.bottom - it->second.original.top,
                            it->second.original.left, it->second.original.top);
            } else {
                LogWinError("SetBounds (restore)");
            }
            g_controlled.erase(it);
            return;
        }
        // Moved but the drag hook missed it: scale down to original size in place.
        const RECT rc = SizeAt(current, it->second.original);
        if (winutil::SetBounds(hwnd, rc)) {
            std::printf("%s: shrank window in place to %ldx%ld at (%ld,%ld)\n",
                        Name().c_str(), rc.right - rc.left, rc.bottom - rc.top,
                        rc.left, rc.top);
        } else {
            LogWinError("SetBounds (shrink)");
        }
        g_controlled.erase(it);
        return;
    }

    // The original to restore is the very first pre-application bounds, so if
    // the window was already snapped, keep its stored original when we now
    // maximize it.
    RECT original = current;
    if (it != g_controlled.end()) {
        original = it->second.original;
    }
    winutil::EnsureRestored(hwnd);
    if (it == g_controlled.end()) {
        if (!winutil::GetBounds(hwnd, original)) {
            LogWinError("GetBounds");
            return;
        }
    }

    bool ok = false;
    const RECT target = MaximizeTarget(hwnd, ok);
    if (!ok) {
        return;
    }
    if (!winutil::SetBounds(hwnd, target)) {
        LogWinError("SetBounds (maximize)");
        return;
    }

    g_controlled[hwnd] = Control{Mode::Maximize, original, target,
                                 HSide::Full, VSide::Full};
    std::printf("%s: maximized window to %ldx%ld at (%ld,%ld)\n",
                Name().c_str(),
                target.right - target.left, target.bottom - target.top,
                target.left, target.top);
}

// ---------------------------------------------------------------------------
// Snap
// ---------------------------------------------------------------------------

const std::string& Snap::ActionName(winutil::Half half) {
    static const std::string kLeft = "snap_left";
    static const std::string kRight = "snap_right";
    static const std::string kTop = "snap_top";
    static const std::string kBottom = "snap_bottom";
    switch (half) {
        case winutil::Half::Left:
            return kLeft;
        case winutil::Half::Right:
            return kRight;
        case winutil::Half::Top:
            return kTop;
        case winutil::Half::Bottom:
            return kBottom;
    }
    return kLeft;
}

Snap::Snap(winutil::Half half) : half_(half) {}

const std::string& Snap::Name() const {
    return ActionName(half_);
}

void Snap::Apply(HWND hwnd) {
    RECT current{};
    if (!winutil::GetBounds(hwnd, current)) {
        LogWinError("GetBounds");
        return;
    }

    // A window we already snapped keeps its region so the next snap composes
    // into a quarter. Anything else (fresh, or maximized by us) currently spans
    // the whole monitor, so it starts from the full region.
    auto it = g_controlled.find(hwnd);
    HSide prevH = HSide::Full;
    VSide prevV = VSide::Full;
    if (it != g_controlled.end() && it->second.mode == Mode::Snap) {
        prevH = it->second.h;
        prevV = it->second.v;
    }

    HSide nextH = HSide::Full;
    VSide nextV = VSide::Full;
    EvolveRegion(half_, prevH, prevV, nextH, nextV);

    bool ok = false;
    const RECT work = MarginedWorkArea(hwnd, ok, Name().c_str());
    if (!ok) {
        return;
    }
    const RECT target = RegionBounds(work, nextH, nextV);

    if (BoundsEqual(current, target)) {
        std::printf("%s: window already in that region; no change.\n",
                    Name().c_str());
        return;
    }

    // Preserve the window's original pre-application bounds across placement
    // switches (e.g. a maximized window that is then snapped) so an eventual
    // drag-untoggle returns to the true original size.
    RECT original{};
    if (it != g_controlled.end()) {
        original = it->second.original;
    } else {
        winutil::EnsureRestored(hwnd);
        if (!winutil::GetBounds(hwnd, original)) {
            LogWinError("GetBounds");
            return;
        }
    }

    if (!winutil::SetBounds(hwnd, target)) {
        LogWinError("SetBounds (snap)");
        return;
    }

    g_controlled[hwnd] = Control{Mode::Snap, original, target, nextH, nextV};
    std::printf("%s: snapped window to %ldx%ld at (%ld,%ld)\n",
                Name().c_str(),
                target.right - target.left, target.bottom - target.top,
                target.left, target.top);
}

// ---------------------------------------------------------------------------
// MoveMonitor
// ---------------------------------------------------------------------------

const std::string& MoveMonitor::ActionName(winutil::Direction dir) {
    static const std::string kLeft = "move_monitor_left";
    static const std::string kRight = "move_monitor_right";
    switch (dir) {
        case winutil::Direction::Left:
            return kLeft;
        case winutil::Direction::Right:
            return kRight;
    }
    return kLeft;
}

MoveMonitor::MoveMonitor(winutil::Direction dir) : dir_(dir) {}

const std::string& MoveMonitor::Name() const {
    return ActionName(dir_);
}

void MoveMonitor::Apply(HWND hwnd) {
    RECT work{};
    if (!winutil::GetNeighborMonitorWorkArea(hwnd, dir_, work)) {
        std::fprintf(stderr, "%s: no monitor to the %s; no change.\n",
                     Name().c_str(),
                     dir_ == winutil::Direction::Left ? "left" : "right");
        return;
    }
    if (!ApplyMargins(work)) {
        std::fprintf(stderr, "%s: margins leave no usable space.\n", Name().c_str());
        return;
    }

    // Carry the window's current region across to the new monitor so a snapped
    // (or quartered) window lands on the same side there. A window we do not
    // control starts from the full region, matching how a fresh snap behaves.
    auto it = g_controlled.find(hwnd);
    HSide h = HSide::Full;
    VSide v = VSide::Full;
    RECT original{};
    if (it != g_controlled.end()) {
        h = it->second.h;
        v = it->second.v;
        original = it->second.original;
    } else {
        winutil::EnsureRestored(hwnd);
        if (!winutil::GetBounds(hwnd, original)) {
            LogWinError("GetBounds");
            return;
        }
    }

    const RECT target = RegionBounds(work, h, v);
    if (!winutil::SetBounds(hwnd, target)) {
        LogWinError("SetBounds (move monitor)");
        return;
    }

    g_controlled[hwnd] = Control{Mode::Snap, original, target, h, v};
    std::printf("%s: moved window %s, placed at %ldx%ld at (%ld,%ld)\n",
                Name().c_str(),
                dir_ == winutil::Direction::Left ? "left" : "right",
                target.right - target.left, target.bottom - target.top,
                target.left, target.top);
}

// ---------------------------------------------------------------------------
// Drag release
// ---------------------------------------------------------------------------

bool IsControlled(HWND hwnd) {
    return g_controlled.find(hwnd) != g_controlled.end();
}

bool ReleaseControl(HWND hwnd) {
    auto it = g_controlled.find(hwnd);
    if (it == g_controlled.end()) {
        return false;
    }
    g_controlled.erase(it);
    std::printf("drag: released window from control.\n");
    return true;
}

// ---------------------------------------------------------------------------
// ActionFactory
// ---------------------------------------------------------------------------

std::unique_ptr<Action> ActionFactory::Create(const std::string& name) {
    if (name == MaximizeToggle::ActionName()) {
        return std::make_unique<MaximizeToggle>();
    }
    for (winutil::Half half : {winutil::Half::Left, winutil::Half::Right,
                               winutil::Half::Top, winutil::Half::Bottom}) {
        if (name == Snap::ActionName(half)) {
            return std::make_unique<Snap>(half);
        }
    }
    for (winutil::Direction dir : {winutil::Direction::Left,
                                   winutil::Direction::Right}) {
        if (name == MoveMonitor::ActionName(dir)) {
            return std::make_unique<MoveMonitor>(dir);
        }
    }
    return nullptr;
}

}  // namespace ffs
