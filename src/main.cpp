#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "Actions.h"
#include "Config.h"
#include "HotkeyManager.h"

namespace {

constexpr UINT kTrayCallback = WM_APP + 1;
constexpr UINT kTrayId = 1;
constexpr UINT kExitCommand = 1001;

const wchar_t kWindowClass[] = L"FakeFullscreenTrayWindow";
const wchar_t kWindowTitle[] = L"FakeFullscreen";

std::string g_exeDir;
HWND g_window = nullptr;

std::string ExeDirectory() {
    char path[MAX_PATH] = {};
    DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string dir(path, len);
    const size_t slash = dir.find_last_of("\\/");
    if (slash != std::string::npos) {
        dir = dir.substr(0, slash);
    }
    return dir;
}

bool AddTrayIcon(HINSTANCE hInstance) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_window;
    nid.uID = kTrayId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = kTrayCallback;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy(nid.szTip, kWindowTitle);
    return Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
}

void RemoveTrayIcon() {
    if (g_window == nullptr) {
        return;
    }
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_window;
    nid.uID = kTrayId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    AppendMenuW(menu, MF_STRING, kExitCommand, L"E&xit FakeFullscreen");

    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(g_window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, g_window, nullptr);
    DestroyMenu(menu);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case kTrayCallback:
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                    ShowTrayMenu();
                    break;
                case WM_LBUTTONDBLCLK:
                    break;
            }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == kExitCommand) {
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_exeDir = ExeDirectory();
    const std::string configPath = g_exeDir + "\\config.ini";

    if (GetFileAttributesA(configPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!config::WriteDefault(configPath)) {
            std::string msg = "Could not write default config to:\n" + configPath;
            MessageBoxA(nullptr, msg.c_str(), "FakeFullscreen", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    config::LoadResult cfg = config::Load(configPath);
    for (const std::string& err : cfg.errors) {
        std::fprintf(stderr, "config warning: %s\n", err.c_str());
    }

    HotkeyManager manager;
    for (const config::Binding& b : cfg.bindings) {
        std::unique_ptr<ffs::Action> action = ffs::ActionFactory::Create(b.actionName);
        if (!action) {
            std::fprintf(stderr, "config: unknown action '%s' (skipped).\n",
                         b.actionName.c_str());
            continue;
        }
        if (!manager.Add(b.modifiers, b.vk, std::move(action))) {
            std::fprintf(stderr, "config: could not bind '%s' (skipped).\n",
                         b.actionName.c_str());
        }
    }

    if (manager.empty()) {
        std::string msg = "No hotkeys were registered.\nCheck config at:\n" + configPath;
        MessageBoxA(nullptr, msg.c_str(), "FakeFullscreen", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kWindowClass;
    RegisterClassExW(&wc);

    g_window = CreateWindowExW(0, kWindowClass, kWindowTitle, WS_OVERLAPPEDWINDOW,
                               0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    if (g_window == nullptr) {
        MessageBoxA(nullptr, "Could not create the tray window.", "FakeFullscreen",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!AddTrayIcon(hInstance)) {
        MessageBoxA(nullptr, "Could not add the tray icon.", "FakeFullscreen",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_HOTKEY) {
            manager.Dispatch(msg.wParam);
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RemoveTrayIcon();
    return 0;
}
