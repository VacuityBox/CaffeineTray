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

auto GetAppDataPath  () -> std::filesystem::path;
auto IsLightTheme    () -> bool;
auto IsSessionLocked () -> bool;

auto ScanProcesses (std::function<bool (HANDLE, DWORD, const std::wstring_view)> checkFn) -> bool;
auto ScanWindows   (std::function<bool (HWND, DWORD, const std::wstring_view)> checkFn, bool onlyVisible = true) -> bool;

auto ToString (std::string str)       -> std::string;
auto ToString (std::wstring str)      -> std::string;
auto ToString (const char* str)       -> std::string;
auto ToString (const wchar_t* str)    -> std::string;
auto ToString (std::string_view str)  -> std::string;
auto ToString (std::wstring_view str) -> std::string;

inline auto ToString () -> std::string
{
    return std::string();
}

template <typename T, typename ...Ts>
auto ToString (T message, Ts ... args) -> std::string
{
    return ToString(message) + ToString(args...);
}

#define SINGLE_INSTANCE_GUARD()                                                               \
    static std::mutex _mutex_##__LINE__;                                                      \
    auto _lock_##__LINE__ = std::unique_lock<std::mutex>(_mutex_##__LINE__, std::defer_lock); \
    if (!_lock_##__LINE__.try_lock()) { return false; }                                       \
    do {} while (0)

} // namespace Caffeine
