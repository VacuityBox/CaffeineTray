#pragma once

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

struct AppInitInfo
{
    HINSTANCE InstanceHandle;
    fs::path  DataDirectory;
    fs::path  SettingsPath;
    fs::path  LogFilePath;
    bool      IsPortable;
};

auto GetAppInitInfo (HINSTANCE hInstance) -> AppInitInfo;

} // namespace Caffeine
