#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <optional>

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
        
        struct
        {
            bool keepDisplayOn;
        } Standard;

        struct
        {
            int scanInterval;
            bool keepDisplayOn;
            std::vector<std::wstring> processNames;
            std::vector<std::wstring> processPaths;
            std::vector<std::wstring> windowTitles;
        } Auto;

        Settings()
            : mode(Mode::Disabled)
        {
            Standard.keepDisplayOn = false;

            Auto.keepDisplayOn = false;
            Auto.scanInterval = 1000;

            Auto.processNames.clear();
            Auto.processPaths.clear();
            Auto.windowTitles.clear();
        }
    } Settings;

    HWND      mWndHandle;
    HINSTANCE mInstance;
    bool      mEnumWindowsRetCode;
    bool      mIsTimerRunning;
    bool      mLightTheme;
    bool      mInitialized;

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

    auto LoadSettings          () -> bool;
    auto SaveSettings          () -> bool;
    auto LaunchSettingsProgram () -> bool;
    static auto UTF8ToUTF16    (const std::string_view str) -> std::optional<std::wstring>;
    static auto UTF16ToUTF8    (const std::wstring_view str) -> std::optional<std::string>;

    // Timer related/used by functions.
    auto ResetTimer       () -> bool;
    auto TimerUpdateProc  () -> void;
    auto ScanProcesses    () -> bool;
    auto ScanWindows      () -> bool;
    auto CheckProcess (const std::wstring_view processImageName) -> bool;
    auto CheckWindow  (const std::wstring_view windowTitle) -> bool;

    auto IsLightTheme () -> bool;

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
