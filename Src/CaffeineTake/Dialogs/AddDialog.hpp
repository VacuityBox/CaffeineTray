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

#include "Helpers/AddEditDialog.hpp"
#include "Helpers/DialogResult.hpp"

namespace CaffeineTake {

class AddDialog : public AddEditDialog<AddDialogResult>
{
    virtual auto OnInit (HWND dlgHandle) -> bool override
    {
        EnableWindow(GetDlgItem(dlgHandle, IDOK), FALSE);
        
        // To make focus work we need to return false.
        SetFocus(GetDlgItem(dlgHandle, IDC_EDITBOX_ADD_EDIT));
        return false;
    }

    virtual auto OnOk () -> bool override
    {
        auto vt = GetValueAndType();
        if (!vt.has_value())
        {
            return false;
        }

        auto [value, type] = vt.value();

        // Check if item already exists.
        if (mItems->Exists(value, type))
        {
            ShowErrorMessageBox(type);

            return false;
        }

        this->mResult.value = value;
        this->mResult.type  = type;

        return true;
    }

public:
    AddDialog (std::shared_ptr<ItemList> items)
        : AddEditDialog (items, L"Add")
    {
    }
};

} // namespace CaffeineTake
