#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <fstream>

namespace Caffeine {

class CaffeineTray
{
    class __declspec(uuid("188C8294-3EB5-45BF-AC59-E4A94D7598AE")) CaffeineNotifyIcon;

    static constexpr auto WM_APP_NOTIFY = static_cast<UINT>(WM_APP + 1); // NotifyIcon messages.
    static constexpr auto IDT_CAFFEINE  = static_cast<UINT>(10001);      // Timer.

    enum class Mode
    {
        Disabled,
        Enabled,

        Auto_Inactive,
        Auto_Active,
    };

    struct Settings
    {
        Mode mode;
        int logLevel;
        
        struct
        {
            bool keepDisplayOn;
        } Standard;

        struct
        {
            int updateInterval;
            bool keepDisplayOn;
            std::vector<std::wstring> processNames;
            std::vector<std::wstring> processPaths;
            std::vector<std::wstring> windowTitles;
        } Auto;        
    } Settings;

    HWND      mWndHandle;
    HINSTANCE mInstance;
    bool      mEnumWindowsRetCode;
    bool      mIsTimerRunning;

    std::wofstream mLogger;

    // Updates icons/strings/power settings/timer. Call after mode change.
    auto UpdateCaffeine  () -> void;
    auto EnableCaffeine  () -> void;    // Prevent computer from sleep.
    auto DisableCaffeine () -> void;    // Allow computer to sleep.
    auto ToggleCaffeine  () -> void;
    auto SetCaffeineMode (Mode mode) -> void;

    auto AddNotifyIcon        () -> bool;
    auto DeleteNotifyIcon     () -> bool;
    auto UpdateNotifyIcon     () -> bool;
    auto LoadIconHelper       (WORD icon) -> HICON;
    auto ShowNotifyContexMenu (HWND hWnd, LONG x, LONG y) -> void;

    auto LoadSettings          () -> void;
    auto LaunchSettingsProgram () -> bool;

    // Timer related/used by functions.
    auto ResetTimer       () -> bool;
    auto TimerUpdateProc  () -> void;
    auto ScanProcesses    () -> bool;
    auto ScanWindows      () -> bool;
    auto CheckProcess (const std::wstring_view processImageName) -> bool;
    auto CheckWindow  (const std::wstring_view windowTitle) -> bool;

    auto Log      () -> std::wostream&;
    auto ModeToString (Mode mode) -> std::wstring_view;

    // Window/timer callbacks.
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
