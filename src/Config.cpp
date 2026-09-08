#include "Config.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace config {

namespace {

const char* kDefaultConfig =
    "# FakeFullscreen hotkey bindings.\n"
    "#\n"
    "# Format:  action = modifiers+key\n"
    "# modifiers: ctrl, alt, shift, win  (combined with '+'; e.g. ctrl+alt+space)\n"
    "# actions:  maximize_toggle, snap_left, snap_right, snap_top, snap_bottom,\n"
    "#           move_monitor_left, move_monitor_right\n"
    "# Lines starting with '#' are comments.\n"
    "\n"
    "maximize_toggle     = ctrl+alt+space\n"
    "snap_left           = ctrl+alt+left\n"
    "snap_right          = ctrl+alt+right\n"
    "snap_top            = ctrl+alt+up\n"
    "snap_bottom         = ctrl+alt+down\n"
    "move_monitor_left   = ctrl+alt+shift+left\n"
    "move_monitor_right  = ctrl+alt+shift+right\n";

std::string Trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const size_t begin = s.find_first_not_of(ws);
    if (begin == std::string::npos) {
        return "";
    }
    const size_t end = s.find_last_not_of(ws);
    return s.substr(begin, end - begin + 1);
}

std::vector<std::string> Split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, delim)) {
        parts.push_back(part);
    }
    return parts;
}

bool IsLetterVk(const std::string& tok, UINT& vk) {
    if (tok.size() == 1) {
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(tok[0])));
        if (c >= 'a' && c <= 'z') {
            vk = 'A' + (c - 'a');
            return true;
        }
        if (c >= '0' && c <= '9') {
            vk = '0' + (c - '0');
            return true;
        }
    }
    return false;
}

bool IsFunctionVk(const std::string& tok, UINT& vk) {
    if (tok.size() >= 2 && tok[0] == 'f') {
        int n = 0;
        for (size_t i = 1; i < tok.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(tok[i]))) {
                return false;
            }
            n = n * 10 + (tok[i] - '0');
        }
        if (n >= 1 && n <= 24) {
            vk = VK_F1 + (n - 1);
            return true;
        }
    }
    return false;
}

bool KeyTokenToVk(const std::string& tok, UINT& vk) {
    struct Named {
        const char* name;
        UINT vk;
    };
    static const Named kNamed[] = {
        {"space", VK_SPACE}, {"enter", VK_RETURN}, {"return", VK_RETURN},
        {"escape", VK_ESCAPE}, {"esc", VK_ESCAPE}, {"tab", VK_TAB},
        {"backspace", VK_BACK}, {"delete", VK_DELETE}, {"insert", VK_INSERT},
        {"home", VK_HOME}, {"end", VK_END}, {"pageup", VK_PRIOR},
        {"pagedown", VK_NEXT},
        {"left", VK_LEFT}, {"right", VK_RIGHT}, {"up", VK_UP}, {"down", VK_DOWN},
    };
    for (const Named& n : kNamed) {
        if (tok == n.name) {
            vk = n.vk;
            return true;
        }
    }
    return IsLetterVk(tok, vk) || IsFunctionVk(tok, vk);
}

bool ModifierToFlag(const std::string& tok, UINT& flag) {
    if (tok == "ctrl" || tok == "control") {
        flag = MOD_CONTROL;
    } else if (tok == "alt") {
        flag = MOD_ALT;
    } else if (tok == "shift") {
        flag = MOD_SHIFT;
    } else if (tok == "win" || tok == "windows" || tok == "meta") {
        flag = MOD_WIN;
    } else {
        return false;
    }
    return true;
}

bool ParseBindingLine(const std::string& line, Binding& out, std::string& err) {
    const size_t eq = line.find('=');
    if (eq == std::string::npos) {
        err = "missing '='";
        return false;
    }
    const std::string action = Trim(line.substr(0, eq));
    const std::string combo = Trim(line.substr(eq + 1));
    if (action.empty() || combo.empty()) {
        err = "empty action or binding";
        return false;
    }

    const std::vector<std::string> parts = Split(combo, '+');
    if (parts.empty()) {
        err = "empty binding";
        return false;
    }
    const std::string keyTok = Trim(parts.back());

    UINT modifiers = 0;
    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        UINT flag = 0;
        if (!ModifierToFlag(Trim(parts[i]), flag)) {
            err = "unknown modifier '" + Trim(parts[i]) + "'";
            return false;
        }
        modifiers |= flag;
    }
    if (modifiers == 0) {
        err = "no modifier specified";
        return false;
    }

    UINT vk = 0;
    if (!KeyTokenToVk(keyTok, vk)) {
        err = "unknown key '" + keyTok + "'";
        return false;
    }

    out.actionName = action;
    out.vk = vk;
    out.modifiers = modifiers;
    return true;
}

}  // namespace

LoadResult Load(const std::string& path) {
    LoadResult result;
    std::ifstream in(path);
    if (!in.is_open()) {
        result.errors.push_back("could not open '" + path + "'");
        return result;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        Binding b;
        std::string err;
        if (ParseBindingLine(trimmed, b, err)) {
            result.bindings.push_back(b);
        } else {
            result.errors.push_back("line " + std::to_string(lineNo) + ": " + err);
        }
    }
    return result;
}

bool WriteDefault(const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << kDefaultConfig;
    return out.good();
}

}  // namespace config
