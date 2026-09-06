#pragma once
#include <windows.h>

#include <memory>
#include <vector>

#include "Actions.h"

// Owns all registered hotkeys and dispatches WM_HOTKEY to the bound action.
class HotkeyManager {
public:
    ~HotkeyManager();

    // Adds a binding; returns false if id assignment or registration fails.
    bool Add(UINT modifiers, UINT vk, std::unique_ptr<ffs::Action> action);

    bool empty() const { return entries_.empty(); }
    size_t size() const { return entries_.size(); }

    // Routes a received WM_HOTKEY. Returns true if the id was handled.
    bool Dispatch(WPARAM id) const;

private:
    struct Entry {
        int id;
        UINT vk;
        UINT modifiers;
        std::unique_ptr<ffs::Action> action;
    };
    std::vector<Entry> entries_;
    int nextId_ = 1;
};
