#include "HotkeyManager.h"

#include <cstdio>
#include <utility>

#include "winutil.h"

// Repeat suppression keeps one physical keypress from firing the action many
// times while the key is held.
constexpr UINT kNoRepeat = 0x4000;  // MOD_NOREPEAT

HotkeyManager::~HotkeyManager() {
    for (const Entry& e : entries_) {
        UnregisterHotKey(nullptr, e.id);
    }
}

bool HotkeyManager::Add(UINT modifiers, UINT vk, std::unique_ptr<ffs::Action> action) {
    if (!action) {
        return false;
    }
    const int id = nextId_++;
    if (!RegisterHotKey(nullptr, id, modifiers | kNoRepeat, vk)) {
        std::fprintf(stderr, "RegisterHotKey failed for '%s' (error %lu).\n",
                     action->Name().c_str(), GetLastError());
        return false;
    }
    Entry e;
    e.id = id;
    e.vk = vk;
    e.modifiers = modifiers;
    e.action = std::move(action);
    entries_.push_back(std::move(e));
    return true;
}

bool HotkeyManager::Dispatch(WPARAM id) const {
    for (const Entry& e : entries_) {
        if (static_cast<WPARAM>(e.id) == id) {
            HWND fg = winutil::ForegroundWindow();
            if (!winutil::IsValid(fg)) {
                std::fprintf(stderr, "%s: no valid foreground window; ignoring.\n",
                             e.action->Name().c_str());
                return true;
            }
            e.action->Apply(fg);
            return true;
        }
    }
    return false;
}
