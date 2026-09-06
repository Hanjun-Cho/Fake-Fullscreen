#pragma once
#include <windows.h>

#include <string>
#include <vector>

namespace config {

struct Binding {
    std::string actionName;
    UINT vk = 0;
    UINT modifiers = 0;
};

struct LoadResult {
    std::vector<Binding> bindings;
    std::vector<std::string> errors;
};

// Parses "action = modifiers+key" lines (and writes errors for bad lines).
LoadResult Load(const std::string& path);

// Writes a default binding template to path. Returns false on failure.
bool WriteDefault(const std::string& path);

}  // namespace config
