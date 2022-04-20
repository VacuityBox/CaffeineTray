#pragma once

#include "AppInitInfo.hpp"
#include "ExecutionState.hpp"
#include "Dialogs/CaffeineSettings.hpp"
#include "IconPack.hpp"
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
    
    bool            mLightTheme;
    bool            mInitialized;    
    bool            mSessionLocked;
    fs::path        mSettingsFilePath;
    ExecutionState  mCaffeine;
    std::mutex      mCaffeineMutex;
    ProcessScanner  mProcessScanner;
    WindowScanner   mWindowScanner;
    Timer           mScannerTimer;
    fs::path        mCustomIconsPath;
    IconPack        mIconPack;

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

    auto UpdateIcon      (bool autoActive = false) -> bool;
    auto UpdateAppIcon   ()          -> void;
    auto LoadIconHelper  (UINT icon) -> HICON;
    auto LoadSquaredIcon (UINT icon) -> HICON;
    auto LoadCustomIcon  (UINT icon) -> HICON;

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
    CaffeineTray  (const AppInitInfo& info);
    ~CaffeineTray ();

    auto Init () -> bool;
};

} // namespace Caffeine
