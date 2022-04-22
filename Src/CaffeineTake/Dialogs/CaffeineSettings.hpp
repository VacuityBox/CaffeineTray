// CaffeineTake - Keep your computer awake.
// 
// Copyright (c) 2020-2021 VacuityBox
// Copyright (c) 2022      serverfailure71
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 
// SPDX-License-Identifier: GPL-3.0-only

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

namespace CaffeineTake {

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

} // namespace CaffeineTake
