#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Caffeine {

auto UTF8ToUTF16 (const std::string_view str) -> std::optional<std::wstring>;
auto UTF16ToUTF8 (const std::wstring_view str) -> std::optional<std::string>;

auto GetAppDataPath () -> std::filesystem::path;
auto IsLightTheme   () -> bool;

auto ScanProccess (std::function<bool (HANDLE, DWORD, const std::wstring_view)> checkFn) -> bool;
auto ScanWindows  (std::function<bool (HWND, DWORD, const std::wstring_view)> checkFn, bool onlyVisible = true) -> bool;

} // namespace Caffeine
