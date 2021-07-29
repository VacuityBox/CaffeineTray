#pragma once

#include "Dialog.hpp"
#include "Helpers/IconCache.hpp"
#include "Helpers/ItemList.hpp"
#include "Helpers/RunningProcess.hpp"
#include "Settings.hpp"
#include "resource.h"

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

class CaffeineSettings final : public Dialog<Settings>
{
    // Construction order is important.
    // Settings -> IconCache -> RunningProcesses -> ItemList
    std::shared_ptr<Settings>           mCurrentSettings;
    std::shared_ptr<IconCache>          mIconCache;
    std::shared_ptr<RunningProcessList> mRunningProcesses;
    std::shared_ptr<ItemList>           mItems;

    HWND mListViewItems;
    HWND mButtonAdd;
    HWND mButtonWizard;
    HWND mButtonEdit;
    HWND mButtonRemove;

    bool mWarningShowed;

    virtual auto OnInit    (HWND dlgHandle)               -> bool override;
    virtual auto OnOk      ()                             -> bool override;
    virtual auto OnCancel  ()                             -> bool override;
    virtual auto OnCommand (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnNotify  (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnClose   ()                             -> void override;

    auto InsertItem (Item item)            -> bool;
    auto ModifyItem (int index, Item item) -> bool;
    auto RemoveItem (int index)            -> bool;

    auto Refresh  () -> void;
    auto FindIcon (const std::wstring& name, ItemType type) -> int;

    auto GetSelectedIndex (bool deselectAfter = false) -> int;
    auto SetSelectedItem  (int index) -> bool;

    auto DisplayWarning () -> void;
    auto HideWarning    () -> void;

    CaffeineSettings            (const CaffeineSettings&) = delete;
    CaffeineSettings& operator= (const CaffeineSettings&) = delete;

public:
    CaffeineSettings (std::shared_ptr<Settings> currentSettings)
        : Dialog            (IDD_SETTINGS)
        , mCurrentSettings  (currentSettings)
        , mIconCache        (std::make_shared<IconCache>())
        , mRunningProcesses (std::make_shared<RunningProcessList>(mIconCache))
        , mItems            (std::make_shared<ItemList>(currentSettings, mRunningProcesses))
        , mListViewItems    (NULL)
        , mButtonAdd        (NULL)
        , mButtonWizard     (NULL)
        , mButtonEdit       (NULL)
        , mButtonRemove     (NULL)
        , mWarningShowed    (false)
    {
    }
};

} // namespace Caffeine
