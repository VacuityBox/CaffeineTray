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

class EditDialog : public AddEditDialog<EditDialogResult>
{
    int mIndexToInitElement;

    virtual auto OnInit (HWND dlgHandle) -> bool override
    {
        if (mIndexToInitElement < 0 || mItems->Count() <= mIndexToInitElement)
        {
            return true;
        }

        const auto& items = mItems->GetItems();
        const auto& value = items[mIndexToInitElement].value;
        const auto& type  = items[mIndexToInitElement].type;

        // Fill the controls with init data.
        auto editControl = GetDlgItem(dlgHandle, IDC_EDITBOX_ADD_EDIT);
        if (type != ItemType::Invalid)
        {
            SetCheckedRadio(dlgHandle, type);
        
            Edit_SetText(editControl, value.c_str());
            Edit_SetSel(editControl, value.length(), value.length());
        
            EnableWindow(GetDlgItem(dlgHandle, IDOK), TRUE);
        }
        
        // To make focus work we need to return false.
        SetFocus(editControl);
        return false;
    }

    virtual auto OnOk () -> bool override
    {
        auto vt = GetValueAndType();
        if (!vt.has_value())
        {
            return true;
        }

        auto [value, type] = vt.value();

        // Check if item already exists.
        const auto& items = mItems->GetItems();
        for (auto index = 0; index < items.size(); index += 1)
        {
            // Skip check of editing element.
            if (index == mIndexToInitElement)
            {
                continue;
            }

            const auto& item = items.at(index);
            if (type == item.type && value == item.value)
            {
                ShowErrorMessageBox(item.type);
                return false;
            }
        }

        // Update the result.
        this->mResult.value = value;
        this->mResult.type  = type;

        return true;
    }

public:
    EditDialog (std::shared_ptr<ItemList> items, int indexToInitElement)
        : AddEditDialog       (items, L"Edit")
        , mIndexToInitElement (indexToInitElement)
    {
    }
};

} // namespace CaffeineTake
