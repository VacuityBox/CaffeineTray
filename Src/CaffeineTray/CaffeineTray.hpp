#pragma once

#include "ExecutionState.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "Logger.hpp"
#include "NotifyIcon.hpp"
#include "Scanner.hpp"
#include "Settings.hpp"
#include "Timer.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

class CaffeineTray : public NotifyIcon
{
    std::shared_ptr<Settings> mSettings;
    std::shared_ptr<Logger>   mLogger;

    bool            mLightTheme;
    bool            mInitialized;
    bool            mSettingsChanged;
    bool            mSessionLocked;
    fs::path        mSettingsFilePath;
    fs::path        mLoggerFilePath;
    ExecutionState  mCaffeine;
    std::mutex      mCaffeineMutex;
    ProcessScanner  mProcessScanner;
    WindowScanner   mWindowScanner;
    Timer           mScannerTimer;

    virtual auto OnCreate      () -> bool override;
    virtual auto OnDestroy     () -> void override;
    virtual auto OnCommand     (WPARAM, LPARAM) -> bool override;
    virtual auto OnClick       (WPARAM, LPARAM) -> bool override;
    virtual auto OnContextMenu (WPARAM, LPARAM) -> bool override;

    virtual auto CustomDispatch (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT override;
    
    auto EnableCaffeine        () -> bool;
    auto DisableCaffeine       () -> bool;
    auto UpdateExecutionState  (bool activate = false) -> void;
    auto ToggleCaffeineMode    () -> void;
    auto SetCaffeineMode       (CaffeineMode mode) -> void;

    auto UpdateIcon () -> bool;
    auto LoadIconHelper (UINT icon) -> HICON;

    auto LoadSettings () -> bool;
    auto SaveSettings () -> bool;

    auto ResetTimer  () -> bool;
    auto TimerUpdate () -> void;

    auto ShowCaffeineSettings () -> bool;
    auto ShowAboutDialog      () -> bool;

    auto Log (std::string message) -> void;
    
    CaffeineTray (const CaffeineTray&) = delete;
    CaffeineTray (CaffeineTray&&)      = delete;
    CaffeineTray& operator = (const CaffeineTray&) = delete;
    CaffeineTray& operator = (CaffeineTray&&)      = delete;

public:
    CaffeineTray  (HINSTANCE hInstance);
    ~CaffeineTray ();

    auto Init () -> bool;
};

} // namespace Caffeine
