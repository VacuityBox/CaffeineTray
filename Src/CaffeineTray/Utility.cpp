#include "Utility.hpp"

#include <array>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <ShlObj.h>
#include <VersionHelpers.h>
#include <wtsapi32.h>

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

auto IsSessionLocked () -> bool
{
    auto wtsinfo  = std::make_unique<WTSINFOEXW*>();
    auto retBytes = DWORD{ 0 };

    auto retCode = WTSQuerySessionInformationW(
        WTS_CURRENT_SERVER_HANDLE,
        WTS_CURRENT_SESSION,
        WTSSessionInfoEx,
        reinterpret_cast<LPWSTR*>(wtsinfo.get()),
        &retBytes
    );

    if (!retCode)
        return false;

    auto isLocked = false;
    auto rawPtr   = *wtsinfo.get();
    if (rawPtr)
    {
        // WTS_SESSIONSTATE_LOCK and WTS_SESSIONSTATE_UNLOCK are reversed on Win7.
        // On newer versions it's fixed.
        if (IsWindows8OrGreater())
        {
            isLocked = rawPtr->Data.WTSInfoExLevel1.SessionFlags == WTS_SESSIONSTATE_LOCK;
        }
        else
        {
            isLocked = rawPtr->Data.WTSInfoExLevel1.SessionFlags == WTS_SESSIONSTATE_UNLOCK;
        }
    }

    WTSFreeMemory(*wtsinfo);

    return isLocked;
}

auto ScanProcesses (std::function<bool (HANDLE, DWORD, const std::wstring_view)> checkFn) -> bool
{
    // Get the list of process identifiers (PID's).
    const auto PROCESS_LIST_MAX_SIZE = 2048;

    auto processList   = std::vector<DWORD>(PROCESS_LIST_MAX_SIZE);
    auto bytesReturned = DWORD{ 0 };
    if (!EnumProcesses(processList.data(), static_cast<DWORD>(processList.size()), &bytesReturned))
    {
        //Log("EnumProcesses() failed");
        return false;
    }
    auto numberOfProccesses = bytesReturned / sizeof(DWORD);

    // Loop through running processes.
    for (auto i = unsigned long{ 0 }; i < numberOfProccesses; ++i)
    {
        auto pid = processList[i];
        if (pid != 0)
        {
            auto processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!processHandle)
            {
                continue;
            }

            // Read process executable path.
            auto imageName = std::array<wchar_t, MAX_PATH>{ 0 };
            auto size      = DWORD{ MAX_PATH };
            if (QueryFullProcessImageNameW(processHandle, 0, imageName.data(), &size))
            {
                // Execute callback.
                if (checkFn(processHandle, pid, imageName.data()))
                {
                    CloseHandle(processHandle);
                    return true;
                }
            }
            
            CloseHandle(processHandle);
        }
    }

    return false;
}

auto ScanWindows (std::function<bool (HWND, DWORD, const std::wstring_view)> checkFn, bool onlyVisible) -> bool
{
    #define ERROR_USER_CALLBACK_SUCCESS (1 << 29) // bit 29 for user errors

    struct EnumWindowsProcData
    {
        decltype(onlyVisible) onlyVisible;
        decltype(checkFn)     checkFn;
    };

    auto enumWindowsProcData = EnumWindowsProcData {onlyVisible, checkFn};
    auto enumWindowsProc = [](HWND hWnd, LPARAM lParam) -> BOOL {
        auto payload     = reinterpret_cast<EnumWindowsProcData*>(lParam);
        auto onlyVisible = payload->onlyVisible;
        auto callbackFn  = payload->checkFn;

        if (onlyVisible && (!IsWindowVisible(hWnd) && !IsIconic(hWnd)))
        {
            return TRUE;
        }

        auto length = GetWindowTextLength(hWnd);
        if (length > 0)
        {
            const auto size = static_cast<size_t>(length) + 1;

            auto title = std::wstring(size, L'\0');
            GetWindowTextW(hWnd, title.data(), size);

            auto pid = DWORD{0};
            GetWindowThreadProcessId(hWnd, &pid);

            // Execute callback.            
            if (callbackFn(hWnd, pid, title.data()))
            {
                SetLastError(ERROR_USER_CALLBACK_SUCCESS);
                return FALSE;
            }
        }

        return TRUE;
    };

    if (!EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&enumWindowsProcData)))
    {
        // Check if callback returned true.
        const auto error = GetLastError();
        if (error == ERROR_USER_CALLBACK_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

auto ToString (std::string str) -> std::string
{
    return str;
}

auto ToString (std::wstring str) -> std::string
{
    return ToString(str.c_str());
}

auto ToString (const char* str) -> std::string
{
    return std::string(str);
}

auto ToString (std::string_view str)  -> std::string
{
    return std::string(str);
}

auto ToString (std::wstring_view str) -> std::string
{
    return ToString(str.data());
}

auto ToString (const wchar_t* str) -> std::string
{
    auto utf8 = UTF16ToUTF8(str);
    if (utf8)
    {
        return utf8.value();
    }

    return "";
}

} // namespace Caffeine
