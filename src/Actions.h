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

// Toggles the focused window between filling its monitor's work area (inset
// by margins) and its original bounds. First Apply stores the original bounds;
// a second Apply restores them.
class MaximizeToggle : public Action {
public:
    static const std::string& ActionName();

    MaximizeToggle();
    ~MaximizeToggle() override;

    const std::string& Name() const override { return ActionName(); }
    void Apply(HWND hwnd) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Moves the focused window into the given half of its monitor's work area,
// respecting margins. Idempotent: does nothing if the window is already in
// that half. Does not toggle/restore.
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

}  // namespace ffs
