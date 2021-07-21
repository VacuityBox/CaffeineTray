#include "Utility.hpp"

#include <array>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>

namespace Caffeine {

auto UTF8ToUTF16 (const std::string_view str) -> std::optional<std::wstring>
{
    // Get size.
    auto size = ::MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0
    );

    if (size <= 0)
    {
        return std::nullopt;
    }

    // Convert.
    auto utf16 = std::wstring(size, L'\0');
    auto ret = ::MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        utf16.data(),
        static_cast<int>(utf16.size())
    );

    if (ret == 0)
    {
        return std::nullopt;
    }

    return utf16;
}

auto UTF16ToUTF8 (const std::wstring_view str) -> std::optional<std::string>
{
    // Get size.
    auto size = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size <= 0)
    {
        return std::nullopt;
    }
    
    // Convert.
    auto utf8 = std::string(size, '\0');
    auto ret = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        utf8.data(),
        static_cast<int>(utf8.size()),
        nullptr,
        nullptr
    );

    if (ret == 0)
    {
        return std::nullopt;
    }

    return utf8;
}

auto GetAppDataPath () -> std::filesystem::path
{
    auto appDataPath = std::array<wchar_t, MAX_PATH>();
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, appDataPath.data())))
    {
        const auto dir = std::wstring(appDataPath.data());
        return std::filesystem::path(dir);
    }

    return std::filesystem::path();
}

auto IsLightTheme () -> bool
{
    auto data     = DWORD{ 0 };
    auto dataSize = DWORD{ sizeof(data) };
    auto status   = ::RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"SystemUsesLightTheme",
        RRF_RT_DWORD,
        NULL,
        &data,
        &dataSize
    );

    if (status == ERROR_SUCCESS)
        return data;

    return false;
}

} // namespace Caffeine
