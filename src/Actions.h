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
// bounds restores the original size and position; dragging the window untoggles
// it via a custom drag (see IsControlled / UntoggleForDrag).
class MaximizeToggle : public Action {
public:
    static const std::string& ActionName();

    MaximizeToggle() = default;
    ~MaximizeToggle() override = default;

    const std::string& Name() const override { return ActionName(); }
    void Apply(HWND hwnd) override;
};

// Moves the focused window into the given half of its monitor's work area,
// respecting margins. Idempotent and one-way: does not restore. The window
// becomes "controlled" so dragging it untoggles to its original size.
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

// Builds the compiled-in set of actions by name. Adding a new action kind
// means adding a registration here.
class ActionFactory {
public:
    static std::unique_ptr<Action> Create(const std::string& name);
};

// Whether the window is currently under application control (maximized or
// snapped by this app).
bool IsControlled(HWND hwnd);

// Called when a manual drag begins on a controlled window (click on its caption).
// Resizes the window to its original size, keeping the point under `cursor`
// fixed so it follows the mouse, and removes it from control. On success fills
// `out` with the resulting bounds and returns true.
bool UntoggleForDrag(HWND hwnd, const POINT& cursor, RECT& out);

}  // namespace ffs
