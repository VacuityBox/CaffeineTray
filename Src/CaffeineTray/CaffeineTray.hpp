#pragma once

#include "ExecutionState.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "Logger.hpp"
#include "Scanner.hpp"
#include "Settings.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

class CaffeineTray
{
    static constexpr auto WM_APP_NOTIFY = static_cast<UINT>(WM_APP + 1); // NotifyIcon messages.
    static constexpr auto IDT_CAFFEINE  = static_cast<UINT>(10001);      // Timer.

    std::shared_ptr<Settings> mSettings;

    HWND            mWndHandle;
    HINSTANCE       mInstance;
    HANDLE          mReloadEvent;
    HANDLE          mReloadThread;
    bool            mEnumWindowsRetCode;
    bool            mIsTimerRunning;
    bool            mLightTheme;
    bool            mInitialized;
    fs::path        mSettingsFilePath;
    fs::path        mLoggerFilePath;
    ExecutionState  mCaffeine;
    Logger          mLogger;
    ProcessScanner  mProcessScanner;
    WindowScanner   mWindowScanner;
    
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
    auto ReloadSettings        () -> void;

    // Timer related/used by functions.
    auto ResetTimer       () -> bool;
    auto TimerUpdateProc  () -> void;

    auto Log (std::string message) -> void;

    auto ShowCaffeineSettings ();
    auto ShowAboutDialog      ();
    
    // Window/timer callbacks.
    static auto CALLBACK ReloadThreadProc(LPVOID lParam) -> DWORD;
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
