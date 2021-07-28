#pragma once

#include "Caffeine.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "Logger.hpp"
#include "Settings.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <filesystem>

namespace fs = std::filesystem;

namespace Caffeine {

class CaffeineTray
{
    static constexpr auto WM_APP_NOTIFY = static_cast<UINT>(WM_APP + 1); // NotifyIcon messages.
    static constexpr auto IDT_CAFFEINE  = static_cast<UINT>(10001);      // Timer.

    static constexpr auto CAFFEINE_LOG_FILENAME      = L"CaffeineTray.log";
    static constexpr auto CAFFEINE_SETTINGS_FILENAME = L"CaffeineTray.json";

    HWND       mWndHandle;
    HINSTANCE  mInstance;
    HANDLE     mReloadEvent;
    HANDLE     mReloadThread;
    bool       mEnumWindowsRetCode;
    bool       mIsTimerRunning;
    bool       mLightTheme;
    bool       mInitialized;
    fs::path   mSettingsFilePath;
    fs::path   mLoggerFilePath;
    Caffeine   mCaffeine;
    Logger     mLogger;
    Settings   mSettings;
    
    // Updates icons/strings/power settings/timer. Call after mode change.
    auto Update () -> void;

    auto EnableCaffeine  () -> void;    // Prevent computer from sleep.
    auto DisableCaffeine () -> void;    // Allow computer to sleep.
    auto ToggleCaffeine  () -> void;
    auto SetCaffeineMode (CaffeineMode mode) -> void;

    auto AddNotifyIcon        () -> bool;
    auto DeleteNotifyIcon     () -> bool;
    auto UpdateNotifyIcon     () -> bool;
    auto LoadIconHelper       (WORD icon) -> HICON;
    auto ShowNotifyContexMenu (HWND hWnd, LONG x, LONG y) -> void;

    auto LoadSettings          () -> bool;
    auto SaveSettings          () -> bool;
    auto LaunchSettingsProgram () -> bool;
    auto ReloadSettings        () -> void;

    // Timer related/used by functions.
    auto ResetTimer       () -> bool;
    auto TimerUpdateProc  () -> void;
    auto ScanProcesses    () -> bool;
    auto ScanWindows      () -> bool;
    auto CheckProcess (const std::wstring_view processImageName) -> bool;
    auto CheckWindow  (const std::wstring_view windowTitle) -> bool;

    auto Log (std::string message) -> void;
    auto ModeToString (CaffeineMode mode) -> std::wstring_view;

    // Window/timer callbacks.
    static auto CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) -> INT_PTR;
    static auto CALLBACK ReloadThreadProc(LPVOID lParam) -> DWORD;
    static auto CALLBACK EnumWindowsProc (HWND hWnd, LPARAM lParam) -> BOOL;
    static auto CALLBACK WndProc         (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT;

    CaffeineTray (const CaffeineTray&) = delete;
    CaffeineTray (CaffeineTray&&)      = delete;
    CaffeineTray& operator = (const CaffeineTray&) = delete;
    CaffeineTray& operator = (CaffeineTray&&)      = delete;

public:
    CaffeineTray  ();
    ~CaffeineTray ();

    auto Init (HINSTANCE hInstance) -> bool;
};

} // namespace Caffeine
