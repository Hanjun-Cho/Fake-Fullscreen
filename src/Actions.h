#pragma once
#include <windows.h>

#include <memory>
#include <string>

#include "winutil.h"

namespace ffs {

// A hotkey action. Instances are created per bound hotkey via ActionFactory.
class Action {
public:
    virtual ~Action() = default;
    virtual void Apply(HWND hwnd) = 0;
    virtual const std::string& Name() const = 0;
};

// Maximizes the focused window to its monitor's work area (minus margins).
// The window becomes "controlled". Pressing again while still at the maximized
// bounds restores the original size and position; dragging the window by its
// title bar releases it from control (see IsControlled / ReleaseControl).
class MaximizeToggle : public Action {
public:
    static const std::string& ActionName();

    MaximizeToggle() = default;
    ~MaximizeToggle() override = default;

    const std::string& Name() const override { return ActionName(); }
    void Apply(HWND hwnd) override;
};

// Pins the focused window to one side of a single axis of its monitor's work
// area, respecting margins. Snaps are cumulative: a perpendicular snap turns
// the window into a quarter, and re-pressing the side the window already hugs
// expands it back toward full. The window becomes "controlled" so dragging it
// by its title bar releases it from control (no resize).
class Snap : public Action {
public:
    static const std::string& ActionName(winutil::Half half);

    explicit Snap(winutil::Half half);
    ~Snap() override = default;

    const std::string& Name() const override;
    void Apply(HWND hwnd) override;

private:
    winutil::Half half_;
};

// Moves the focused window to the monitor adjacent in the given horizontal
// direction, keeping its current snapped region (full, half, or quarter) so it
// lands on the corresponding side of the new monitor's work area. A window not
// under application control spans the full new monitor. The window stays
// "controlled", so dragging it by its title bar releases it as usual.
class MoveMonitor : public Action {
public:
    static const std::string& ActionName(winutil::Direction dir);

    explicit MoveMonitor(winutil::Direction dir);
    ~MoveMonitor() override = default;

    const std::string& Name() const override;
    void Apply(HWND hwnd) override;

private:
    winutil::Direction dir_;
};

// Builds the compiled-in set of actions by name. Adding a new action kind
// means adding a registration here.
class ActionFactory {
public:
    static std::unique_ptr<Action> Create(const std::string& name);
};

// Whether the window is currently under application control (maximized or
// snapped by this app).
bool IsControlled(HWND hwnd);

// Releases a controlled window from application control without resizing or
// moving it. Called when the user begins dragging the window by its caption so
// it then behaves like a normal window. Returns true if the window was under
// control (and was released).
bool ReleaseControl(HWND hwnd);

}  // namespace ffs
