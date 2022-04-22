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
#include "Helpers/DialogResult.hpp"
#include "Helpers/IconCache.hpp"
#include "Helpers/ItemList.hpp"
#include "Helpers/RunningProcess.hpp"
#include "Settings.hpp"
#include "resource.h"

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <windowsx.h>

namespace {
    namespace fs = std::filesystem;
}

namespace CaffeineTake {

class AddWizardDialog final : public Dialog<AddWizardResult>
{
    std::shared_ptr<ItemList>           mItems;
    std::shared_ptr<IconCache>          mIconCache;
    std::shared_ptr<RunningProcessList> mRunningProcesses;

    virtual auto OnInit    (HWND dlgHandle)               -> bool override;
    virtual auto OnOk      ()                             -> bool override;
    virtual auto OnCancel  ()                             -> bool override;
    virtual auto OnCommand (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnNotify  (WPARAM wParam, LPARAM lParam) -> bool override;
    virtual auto OnClose   ()                             -> void override;

    auto Refresh (HWND listView,  bool select = false) -> void;

public:
    AddWizardDialog (
        std::shared_ptr<ItemList>           items,
        std::shared_ptr<IconCache>          iconCache,
        std::shared_ptr<RunningProcessList> processList
    )
        : Dialog            (IDD_ADD_WIZARD)
        , mItems            (items)
        , mIconCache        (iconCache)
        , mRunningProcesses (processList)
    {
    }
};

} // namespace CaffeineTake
