#include "Actions.h"

#include <cstdio>
#include <unordered_map>

namespace ffs {
namespace {

// Per-edge margin (pixels) inset from each side of the monitor's working area
// before maximizing or snapping, so windows do not touch the screen edges.
constexpr long kDefaultMargin = 15;

bool LogWinError(const char* what) {
    std::fprintf(stderr, "%s failed (error %lu)\n", what, GetLastError());
    return false;
}

bool GetWorkArea(HWND hwnd, RECT& out) {
    return winutil::GetMonitorWorkArea(hwnd, out);
}

// Shrinks a rect inward by the margin on every side.
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

}  // namespace

// ---------------------------------------------------------------------------
// MaximizeToggle
// ---------------------------------------------------------------------------

struct MaximizeToggle::Impl {
    std::unordered_map<HWND, RECT> original;
};

const std::string& MaximizeToggle::ActionName() {
    static const std::string kName = "maximize_toggle";
    return kName;
}

MaximizeToggle::MaximizeToggle() : impl_(std::make_unique<Impl>()) {}
MaximizeToggle::~MaximizeToggle() = default;

void MaximizeToggle::Apply(HWND hwnd) {
    auto it = impl_->original.find(hwnd);
    if (it != impl_->original.end()) {
        if (winutil::SetBounds(hwnd, it->second)) {
            std::printf("%s: restored window to %ldx%ld at (%ld,%ld)\n",
                        Name().c_str(),
                        it->second.right - it->second.left,
                        it->second.bottom - it->second.top,
                        it->second.left, it->second.top);
        } else {
            LogWinError("SetBounds (restore)");
        }
        impl_->original.erase(it);
        return;
    }

    winutil::EnsureRestored(hwnd);

    RECT before{};
    if (!winutil::GetBounds(hwnd, before)) {
        LogWinError("GetBounds");
        return;
    }

    RECT work{};
    if (!GetWorkArea(hwnd, work)) {
        LogWinError("GetWorkArea");
        return;
    }
    RECT target = work;
    if (!ApplyMargins(target)) {
        std::fprintf(stderr, "%s: margins leave no usable space.\n", Name().c_str());
        return;
    }

    if (!winutil::SetBounds(hwnd, target)) {
        LogWinError("SetBounds (maximize)");
        return;
    }

    impl_->original[hwnd] = before;
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
    winutil::EnsureRestored(hwnd);

    RECT work{};
    if (!GetWorkArea(hwnd, work)) {
        LogWinError("GetWorkArea");
        return;
    }
    if (!ApplyMargins(work)) {
        std::fprintf(stderr, "%s: margins leave no usable space.\n", Name().c_str());
        return;
    }
    const RECT target = winutil::HalfBounds(work, half_);

    RECT current{};
    if (!winutil::GetBounds(hwnd, current)) {
        LogWinError("GetBounds");
        return;
    }
    if (BoundsEqual(current, target)) {
        std::printf("%s: window already snapped; no change.\n", Name().c_str());
        return;
    }

    if (!winutil::SetBounds(hwnd, target)) {
        LogWinError("SetBounds (snap)");
        return;
    }

    std::printf("%s: snapped window to %ldx%ld at (%ld,%ld)\n",
                Name().c_str(),
                target.right - target.left, target.bottom - target.top,
                target.left, target.top);
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
    return nullptr;
}

}  // namespace ffs
